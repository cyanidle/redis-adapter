#!/usr/bin/env python3
from abc import ABC, abstractmethod
from argparse import ArgumentParser
import asyncio
from collections import deque
from copy import copy
from dataclasses import dataclass
from enum import IntEnum, auto
from functools import partial
import inspect
import json
import logging
import re
import sys
import pathlib
import os
import traceback
from aioconsole import get_standard_streams
from types import CodeType
from typing import TYPE_CHECKING, AbstractSet, Any, Awaitable, Callable, Deque, Dict, Generic, Iterator, List, Mapping, MutableMapping, Optional, Sequence, Tuple, Type, TypeVar, Union, cast
from typing_extensions import ParamSpec

from pydantic import BaseModel, Extra, Field, validate_model
from pydantic.fields import Undefined
assert sys.version_info.major >= 3
assert sys.version_info.minor >= 9

__all__=("BootParams", "Signal", "JsonDict", "Timer", "Worker", "JsonState", "merge_flat_dicts", "routes", "BindField")
_T = TypeVar("_T")

@dataclass
class BootParams:
    file: str
    settings: dict
    worker_name: str
    ioloop: asyncio.AbstractEventLoop

UniversalCallback = Union[Callable[[_T], Any], Callable[[_T], Awaitable]]
ArglessCallback = Union[Callable[[], Any], Callable[[], Awaitable], Awaitable, partial]
UniversalOrArgless = Union[UniversalCallback[_T], ArglessCallback]

async def execute_universal(func: UniversalCallback, *args, **kwargs):
    if asyncio.iscoroutinefunction(func):
        return await func(*args, **kwargs)
    elif callable(func):
        return func(*args, **kwargs)
    elif asyncio.iscoroutine(func):
        if args:
            raise RuntimeError("Arguments passed to already constructed coroutine object!")
        return await func
    else:
        raise TypeError(f"Can only execute function or coroutine! Got: ({func})")

async def execute_universal_or_argless(func: UniversalOrArgless, *args, **kwargs):
    try:
        found_args = bool(inspect.signature(func).parameters) # type: ignore
    except ValueError:
        found_args = True
    if not found_args:
        await execute_universal(func) # type: ignore
    else:
        await execute_universal(func, *args, **kwargs) # type: ignore

class _boot_CbTypes(IntEnum):
    CORO = auto()
    CORO_FUNC = auto()
    PLAIN = auto()

class _boot_SignalMeta(Generic[_T]):
    __slots__ = ("_cb", "_argless", "_type")
    def __init__(self, cb: UniversalOrArgless[_T]) -> None:
        self._cb = cb
        if inspect.iscoroutine(cb):
            self._type = _boot_CbTypes.CORO
            self._argless = True
            return
        if inspect.iscoroutinefunction(cb):
            self._type = _boot_CbTypes.CORO_FUNC
        elif callable(cb):
            self._type = _boot_CbTypes.PLAIN
        else:
            raise TypeError(f"Invalid type is passed as callback to Signal!")
        sign = inspect.signature(cb)
        self._argless = not sign.parameters
    async def invoke(self, *args):
        if self._argless:
            args = tuple()
        if self._type == _boot_CbTypes.CORO:
            if args:
                raise RuntimeError(f"Signal emit on coroutine: {self._cb} with args (Was already wrapped)")
            await self._cb # type: ignore
        elif self._type == _boot_CbTypes.CORO_FUNC:
            await self._cb(*args) # type: ignore
        else:
            self._cb(*args) # type: ignore
            
class Signal(Generic[_T]):
    __slots__ = ("__targets", )
    def __init__(self) -> None:
        self.__targets: Dict[UniversalOrArgless[_T], _boot_SignalMeta[_T]] = {}
    @property
    def callbacks(self):
        return tuple(self.__targets.keys())
    async def emit(self, *args: _T):
        await asyncio.gather(*(cb.invoke(*args) for cb in self.__targets.values()))
    def receive_with(self, callback: UniversalOrArgless[_T]):
        self.__targets[callback] = _boot_SignalMeta(callback)
    def remove_subscriber(self, callback: UniversalOrArgless[_T]):
        if callback in self.__targets:
            del self.__targets[callback]
    def remove_all(self):
        self.__targets.clear()
    def forward_from(self, target: 'Signal[_T]'):
        target.receive_with(self.emit)


class Timer:
    __slots__ = ['_signal', '_ioloop', '_interval', '_task', '_single_shot', '_start_time', '_active']
    def __init__(self, *, ioloop: Optional[asyncio.AbstractEventLoop] = None) -> None:
        self._signal: Signal[None] = Signal()
        self._interval: int = 0
        self._single_shot = False
        self._ioloop = ioloop or asyncio.get_event_loop_policy().get_event_loop()
        self._task: Optional[asyncio.Task] = None # type: ignore
        self._start_time = 0.
        self._active = False
    def set_single_shot(self, is_single_shot: bool):
        self._single_shot = is_single_shot
    @property
    def timeout(self):
        return self._signal
    @property
    def interval(self):
        return self._interval
    def call_on_timeout(self, target: ArglessCallback):
        self.timeout.receive_with(target)
    def set_interval(self, interval_ms: int):
        self._interval = interval_ms
    def remaining_time(self) -> int:
        rem = self._interval - int(round(1000 * (self._ioloop.time() - self._start_time)))
        if not self._active:
            return -1
        return rem if rem > 0 else 0
    def start(self, interval: Optional[int] = None):
        self.stop()
        self._start_time = self._ioloop.time()
        self._active = True
        if interval is not None:
            self._interval = interval
        self._task: asyncio.Task = self._ioloop.create_task(self._impl_timer_task())
    def stop(self):
        if self._task and not self._task.cancelled():
            self._task.cancel()
        self._active = False
    async def _impl_timer_task(self):
        while not self._single_shot:
            await asyncio.sleep(self._interval / 1000.)
            await self._signal.emit()
            self._start_time = self._ioloop.time()
        self._active = False

class Worker(ABC):
    def __init__(self, params: BootParams) -> None:
        self.__logger = logging.getLogger(params.worker_name)
        self.__tasks: List[asyncio.Task] = []
        self.__name = params.worker_name
        self.__logger = logging.getLogger()
        self.__ioloop = params.ioloop
        self.__jsons: Signal[JsonDict] = Signal()
        self.__was_shutdown: Signal[Worker] = Signal()
        self.__was_shutdown.receive_with(self.__jsons.remove_all)
    def run(self):
        self.on_run()
    @abstractmethod
    def on_run(self) -> None:
        raise NotImplementedError
    @abstractmethod
    async def on_msg(self, msg: 'JsonDict'):
        raise NotImplementedError
    @abstractmethod
    def on_shutdown(self) -> None:
        raise NotImplementedError
    @property
    def was_shutdown(self):
        return self.__was_shutdown
    @property
    def msgs(self):
        return self.__jsons
    @property
    def name(self):
        return self.__name
    def shutdown(self, reason: str = 'Not Given'):
        self.log.warn(f"Shutting down... Reason: {reason}")
        try:
            self.on_shutdown()
        except Exception as e:
            self.log.error("While shutting down:")
            self.log.exception(e)
        for task in self.tasks:
            task.cancel()
        _remove_all = self.create_task(self.__was_shutdown.emit(self))
        _remove_all.add_done_callback(lambda x: self.__was_shutdown.remove_all())
    @property
    def is_busy(self) -> bool:
        return bool(self.tasks)
    @staticmethod
    async def execute(func: UniversalCallback, *args):
        await execute_universal(func)
    def cancel_task(self, task: Optional[asyncio.Task]):
        if task is not None and not task.cancelled():
            self.log.info(f"Cancelling Task: {task.get_name()}")
            task.cancel()
    def create_task(self, func: Union[Awaitable, Callable[..., Awaitable]], name: Optional[str] = None, *args, **kwargs) -> asyncio.Task:
        if asyncio.iscoroutinefunction(func):
            task = self.__ioloop.create_task(func(*args, **kwargs), name=name or func.__name__)
        elif asyncio.iscoroutine(func):
            if args:
                raise RuntimeError("Args passed to already constructed Coroutine!")
            task = self.__ioloop.create_task(func, name=name or func.__name__)
        else:
            raise TypeError(f"create_task should be used with async functions only!")
        task.add_done_callback(self.__on_task_done)
        self.log.info(f"Created task {self.__format_task(task)}")
        self.__tasks.append(task)
        return task
    def cancel_all_tasks(self) -> None:
        self.log.warn(f"Canceling all tasks")
        for task in self.tasks:
            task.cancel()
    @property
    def log(self) -> logging.LoggerAdapter:
        return self.__WorkerLogAdapter(self.__logger, worker = self)
    @property
    def tasks(self) -> List[asyncio.Task]:
        return self.__tasks
    @property
    def ioloop(self) -> asyncio.AbstractEventLoop:
        return self.__ioloop
    async def __restart(self):
        self.log.warn("Waiting 3 seconds...")
        await asyncio.sleep(3)
        self.run()
    def __format_task(self, task: asyncio.Task):
        return f"[{self.__class__.__name__}:{task.get_name()}(...)]"
    def __on_task_done(self, task: asyncio.Task) -> None:
        if task.cancelled():
            self.log.warn(f"Task {self.__format_task(task)} Cancelled!")
        else:
            exc = task.exception()
            if exc:
                self.log.error(f"Task {self.__format_task(task)} Exception! Details: {exc.__class__.__name__}:{exc}")
                self.log.error(f"Traceback: {traceback.print_exception(exc)}")
                self.shutdown("Restarting on exception")
                self.ioloop.create_task(self.__restart())
            else:
                self.log.info(f"Task {self.__format_task(task)} done!")
        self.tasks.remove(task)
    def __repr__(self):
        return f"Worker: {self.name}"
    class __WorkerLogAdapter(logging.LoggerAdapter):
        def __init__(self, logger: logging.Logger, extra: Optional[Mapping[str, object]] = None, *, worker: 'Worker') -> None:
            super().__init__(logger, extra)
            self.worker = worker
        def process(self, msg, kwargs):
            return (f"[{self.worker.__class__.__name__}]: {msg}", kwargs) if self.worker.name else (msg, kwargs)

JsonKey = Union[Sequence[str], str]
JsonItem = Union[str, None, int, float, list, dict, Any]
FlatDict = Dict[str, JsonItem]

def merge_flat_dicts(first: FlatDict, second: FlatDict) -> FlatDict:
    return {**first, **second}

class _JsonDictKeyPart:
    __slots__ = ["key", "as_index"]
    def __init__(self, raw: str) -> None:
        self.key = raw
        self.as_index: int = cast(int, None)
        if len(raw) >= 3:
            is_braced = raw[0] == "[" and raw[-1] == "]"
            if is_braced:
                try:
                    self.as_index = int(raw[1:-1])
                except:
                    pass
    @property
    def wanted_container(self) -> type:
        return list if self.is_index else dict
    def is_in(self, target: dict | list):
        if isinstance(target, dict):
            if self.is_index:
                raise KeyError("Attempt to get by index in dict!")
            return self.key in target
        elif isinstance(target, list):
            if not self.is_index:
                raise KeyError("Attempt to get by key in list!")
            return self.as_index < len(target)
        else:
            return TypeError("Attempt to shadow already existing value")
    def set_in(self, target: dict | list, value: Any):
        if isinstance(target, dict):
            if self.is_index:
                raise KeyError("Attempt to set by index in dict!")
            target[self.key] = value
        elif isinstance(target, list):
            if not self.is_index:
                raise KeyError("Attempt to set by key in list!")
            while len(target) <= self.as_index:
                target.append(None)
            target[self.as_index] = value
        else:
            raise TypeError
    def get_from(self, target: dict | list):
        if isinstance(target, dict) and not self.is_index:
            return target[self.key]
        elif isinstance(target, list) and self.is_index:
            return target[self.as_index]
        else:
            raise KeyError(f"Incompatible key {self.key} with {target.__class__.__name__}:{target}")
    @property
    def is_index(self):
        return self.as_index is not None

class _JsonDictKey:
    __slots__ = ["subkeys"]
    def __init__(self, keys: Union[Sequence[str], str]) -> None:
        if isinstance(keys, str):
            keys = keys.split(":")
        self.subkeys = tuple((_JsonDictKeyPart(subkey) for subkey in keys))
    def __iter__(self) -> Iterator[_JsonDictKeyPart]:
        return self.subkeys.__iter__()

class JsonDict(MutableMapping[JsonKey, JsonItem]):
    __slots__ = ["_dict"]
    def __init__(self, src_dict: Union[FlatDict, dict, Any] = None, **kwargs) -> None:
        if src_dict is None: src_dict = kwargs
        if not isinstance(src_dict, dict):
            raise TypeError("Can only construct JsonDict from dict!")
        self._dict = src_dict
        if self._dict:
            self.nest()
    @property
    def top(self):
        return self._dict
    def nest(self, sep: str = ":"):
        copy = JsonDict()
        for k, v in self._dict.items():
            if isinstance(v, list):
                for item in v:
                    if isinstance(item, dict):
                        item = JsonDict(item)
            copy[tuple(k.split(sep))] = v
        self._dict = copy._dict
    def flattened(self, sep:str = ":") -> FlatDict:
        result = {}
        for iter in self.full_iter():
            result[sep.join(iter.key())] = iter.value()
        return result
    def as_bytes(self, encoding:str = "utf-8"):
        return json.dumps(self._dict, separators=(',', ':')).encode(encoding=encoding)
    @classmethod
    def from_bytes(cls, raw: bytes, encoding:str = "utf-8"):
        return cls(json.loads(raw.decode(encoding=encoding).replace("'", '"')))
    def __eq__(self, other: 'JsonDict') -> bool:
        if len(self._dict) != len(other._dict):
            return False
        for iter in self.full_iter():
            try:
                if iter.value() != other[iter.key()]: return False
            except: return False
        for iter in other.full_iter():
            try:
                if iter.value() != self[iter.key()]: return False
            except: return False
        return True
    def __contains__(self, key: Union[JsonKey, 'JsonDictIterator']) -> bool:
        try:
            if isinstance(key, JsonDictIterator):
                return self.__contains__(key.key())
            _ = self[key]
            return True
        except:
            return False
    def __delitem__(self, key: Union[JsonKey, 'JsonDictIterator']) -> None:
        if isinstance(key, JsonDictIterator):
            self.__delitem__(key.key())
            return
        keys = _JsonDictKey(key)
        current: Union[List, Dict] = self._dict
        for subkey in keys.subkeys[:-1]:
            current = subkey.get_from(current)
        last = keys.subkeys[-1]
        if last.is_index:
            del cast(list, current)[last.as_index]
        else:
            del cast(dict, current)[last.key]
    def get(self, key: Union[JsonKey, 'JsonDictIterator'], default:_T = None) -> Union[JsonItem, _T]:
        try:
            if isinstance(key, JsonDictIterator):
                return self.get(key.key())
            return self.__getitem__(key)
        except:
            return default
    def __getitem__(self, key: Union[JsonKey, 'JsonDictIterator']) -> JsonItem:
        if isinstance(key, JsonDictIterator):
            return self.__getitem__(key.key())
        keys = _JsonDictKey(key)
        current = self._dict
        for subkey in keys:
            current = subkey.get_from(current)
        return current
    def __setitem__(self, key: Union[JsonKey, 'JsonDictIterator'], value: Any) -> None:
        if isinstance(key, JsonDictIterator):
            self.__setitem__(key.key(), value)
            return
        if isinstance(value, JsonDict):
            value = value._dict
        keys = _JsonDictKey(key)
        current: Union[list, dict] = self._dict
        for num, subkey in enumerate(keys.subkeys[:-1]):
            if not subkey.is_in(current) or not isinstance(subkey.get_from(current), (dict, list)):
                subkey.set_in(current, keys.subkeys[num + 1].wanted_container())
            current = subkey.get_from(current)
        last = keys.subkeys[-1]
        if last.is_index:
            while len(current) <= last.as_index:
                cast(list, current).append(None)
            cast(list, current)[last.as_index] = value
        else:
            cast(dict, current)[last.key] = value
    def __len__(self) -> int:
        num = 0
        for _ in self:
            num += 1
        return num
    def full_iter(self) -> 'JsonDictIterator':
        return JsonDictIterator(source=self)
    def __iter__(self) -> 'JsonDictIteratorSimple':
        return JsonDictIteratorSimple(source = self)
    def __str__(self) -> str:
        return str(self._dict)
    def __repr__(self) -> str:
        return repr(self._dict)

@dataclass(slots=True)
class _TraverseState:
    iter: Union[Iterator[str], Iterator[JsonItem]]
    key: Optional[str]
    container: Union[dict, list]
    count: int
    value: JsonItem

class JsonDictIterator(Iterator[JsonItem]):
    __slots__ = ["_history", "_state"]
    def __init__(self, *, source: JsonDict) -> None:
        self._state = _TraverseState(
            source._dict.__iter__(),
            None,
            source._dict,
            -1,
            None
        )
        self._history: Deque[_TraverseState] = deque()
    def value(self) -> JsonItem:
        return self._state.value
    def field(self) -> str:
        return cast(str, self.__key())
    def domain(self) -> JsonKey:
        return cast(JsonKey, tuple(map(lambda state: state.key, self._history)))
    def key(self) -> JsonKey:
        return cast(JsonKey, tuple(map(lambda state: state.key, self._history)) + (self.__key(),))
    def depth(self) -> int:
        return len(self._history) + 1
    def __str__(self) -> str:
        return ":".join(self.key())
    def __repr__(self) -> str:
        return f"JsonDictIterator ({':'.join(self.key())})"
    def __key(self):
        if isinstance(self._state.container, dict):
            return self._state.key
        else:
            return f"[{self._state.count}]"
    def __advance(self):
        if isinstance(self._state.container, dict):
            self._state.key = cast(str, self._state.iter.__next__())
            self._state.value = self._state.container[self._state.key]
        else:
            self._state.count += 1
            self._state.key = f"[{self._state.count}]"
            self._state.value = self._state.iter.__next__() 
    def __pop_state(self):
        self._state = self._history.pop()
    def __push_state(self):
        self._history.append(copy(self._state))
        self._state.container = cast(Union[dict, list], self.value())
        self._state.iter = self._state.container.__iter__()
        self._state.count = -1
    def __iter__(self):
        return self
    def __next__(self):
        try:
            self.__advance()
            if isinstance(self._state.value, (dict, list)):
                self.__push_state()
                if not self._state.container and self._history:
                    self.__pop_state()
                self.__next__()
        except StopIteration:
            if not self._history:
                raise StopIteration
            else:
                self.__pop_state()
                self.__next__()
        return self
    
class JsonDictIteratorSimple(JsonDictIterator):
    def __next__(self) -> str:
        super().__next__()
        return str(self)

BIND_RULES_ATTR = "__bind_rules__"
FIELD_CB_ATTR = "_callback"
FIELD_ROUTE_ATTR = "_route"

class JsonBinding:
    __slots__ = ["rules"]
    def __init__(self, rules: Dict[str, str], **mappings) -> None:
        self.rules = rules
        check = re.compile(r"\{.*\}")
        for rule in self.rules:
            try:
                self.rules[rule] = self.rules[rule].format_map(mappings)
            except KeyError as e:
                raise KeyError(f"Missing route rule for mapping {e}")
        for mapping in self.rules.values():
            found = check.findall(":".join(mapping))
            if found:
                raise KeyError(
                    f"Binding did not receive mapping for these placeholders: {found}. Rules: {self.rules}. Got: {mappings}")
        self.rules = {k:v.split(":") for k,v in self.rules.items()}

    async def receive(self, source: JsonDict, *, strict=False) -> FlatDict:
        result = dict()
        for rule, mapped in self.rules.items():
            if strict:
                result[rule] = source[mapped]
            elif mapped in source:
                result[rule] = source[mapped]
        return result

    def send(self, source: FlatDict, *, strict=False) -> JsonDict:
        result = JsonDict()
        for rule, mapped in self.rules.items():
            result[mapped] = source[rule] if strict else source.get(rule)
        return result

class JsonState(BaseModel, extra=Extra.allow):
    def __init__(self, binding: Optional[JsonBinding] = None, **mappings) -> None:
        super().__init__()
        self._callbacks: Dict[str, Tuple[UniversalCallback]] = {}
        for name, field in self.__fields__.items():
            if FIELD_ROUTE_ATTR in field.field_info.extra:
                new_path = field.field_info.extra[FIELD_ROUTE_ATTR]
                if new_path is not None:
                    was = getattr(self.__class__, BIND_RULES_ATTR, {})
                    setattr(self.__class__, BIND_RULES_ATTR, {**was, name: new_path})
            if FIELD_CB_ATTR in field.field_info.extra:
                if not name in self._callbacks:
                    self._callbacks[name] = tuple()    
                self._callbacks[name] = (field.field_info.extra[FIELD_CB_ATTR], ) + self._callbacks[name] # type: ignore
        self._binding = binding or JsonBinding(self._bind_rules(), **mappings)
        if self._binding is None:
            raise TypeError("Binding not provided!")
        for field_name in self.dict():
            if not field_name in self._binding.rules:
                raise KeyError(f"Field '{field_name}' not provided by binding!")
    async def update(self, source: JsonDict):
        received = await self._binding.receive(source)
        await self.__refresh(**received)

    def send(self) -> JsonDict:
        return self._binding.send(self.dict())

    @classmethod
    def _bind_rules(cls) -> Dict[str, str]:
        res = getattr(cls, BIND_RULES_ATTR, None)
        if res is None:
            raise TypeError(
                f"Attempt to create Bindable without provided JsonBinding or Rules")
        return res
    
    async def __refresh(self, **data):
        """Refresh the internal attributes with new data."""
        values, fields, error = validate_model(self.__class__, data)
        if error:
            logging.getLogger("bindings").exception(error)
            return
        for name in fields:
            value = values[name]
            setattr(self, name, value)
            if name in self._callbacks:
                await asyncio.gather(*(execute_universal_or_argless(cb, value) for cb in self._callbacks[name]))

    def bind(self, field_name: str, callback: UniversalOrArgless):
        if not field_name in self.dict().keys():
            raise KeyError(
                f"Field {field_name} does not exist in Bindable:{self.__class__.__name__}")
        if not field_name in self._callbacks:
            self._callbacks[field_name] = tuple()    
        self._callbacks[field_name] = self._callbacks[field_name] + (callback,)  # type: ignore

    def json(self, *, include=None,
             exclude=None,
             by_alias: bool = False,
             skip_defaults: Optional[bool] = None,
             exclude_unset: bool = False,
             exclude_defaults: bool = False,
             exclude_none: bool = False,
             encoder: Optional[Callable[[Any], Any]] = None,
             models_as_dict: bool = True,
             **dumps_kwargs: Any) -> str:
        return super().json(
            include=include,
            exclude=set(exclude if exclude is not None else []).union({"_binding", "_callbacks"}),
            by_alias=by_alias,
            skip_defaults=skip_defaults,
            exclude_unset=exclude_unset,
            exclude_defaults=exclude_defaults,
            exclude_none=exclude_none,
            encoder=encoder,
            models_as_dict=models_as_dict,
            **dumps_kwargs)

    def dict(self, *, include=None,
            exclude=None,
            by_alias: bool = False,
            skip_defaults: Optional[bool] = None,
            exclude_unset: bool = False,
            exclude_defaults: bool = False,
            exclude_none: bool = False) -> Dict[str, Any]:
        return super().dict(
            include=include,
            exclude=set(exclude if exclude is not None else []).union({"_binding", "_callbacks"}),
            by_alias=by_alias,
            skip_defaults=skip_defaults,
            exclude_unset=exclude_unset,
            exclude_defaults=exclude_defaults,
            exclude_none=exclude_none)


def routes(**bind_paths: str) -> Callable[[Type[_T]], Type[_T]]:
    def routes_impl(bindable: Type[_T]) -> Type[_T]:
        was = JsonDict(getattr(bindable, BIND_RULES_ATTR, {}))
        setattr(bindable, BIND_RULES_ATTR, {**was, **bind_paths})
        return bindable
    return routes_impl

def BindField (
    route: Optional[str] = None,
    *,
    default: Any = Undefined,
    on_update: Optional[UniversalOrArgless] = None,
    default_factory: Optional[Callable] = None,
    alias: Optional[str] = None,
    title: Optional[str] = None,
    description: Optional[str] = None,
    exclude: Optional[AbstractSet[Union[str, int]]] = None,
    include: Optional[AbstractSet[Union[str, int]]] = None,
    const: Optional[bool] = None,
    gt: Optional[float] = None,
    ge: Optional[float] = None,
    lt: Optional[float] = None,
    le: Optional[float] = None,
    multiple_of: Optional[float] = None,
    allow_inf_nan: Optional[bool] = None,
    max_digits: Optional[int] = None,
    decimal_places: Optional[int] = None,
    min_items: Optional[int] = None,
    max_items: Optional[int] = None,
    unique_items: Optional[bool] = None,
    min_length: Optional[int] = None,
    max_length: Optional[int] = None,
    allow_mutation: bool = True,
    regex: Optional[str] = None,
    discriminator: Optional[str] = None,
    repr: bool = True,
    **extra: Any,
):
    kwargs = {
        "default_factory": default_factory,
        "alias": alias,
        "title": title,
        "description": description,
        "exclude": exclude,
        "include": include,
        "const": const,
        "gt": gt,
        "ge": ge,
        "lt": lt,
        "le": le,
        "multiple_of": multiple_of,
        "allow_inf_nan": allow_inf_nan,
        "max_digits": max_digits,
        "decimal_places": decimal_places,
        "min_items": min_items,
        "max_items": max_items,
        "unique_items": unique_items,
        "min_length": min_length,
        "max_length": max_length,
        "allow_mutation": allow_mutation,
        "regex": regex,
        "discriminator": discriminator,
        "repr": repr,
        **extra,
        FIELD_ROUTE_ATTR: route, 
        FIELD_CB_ATTR: on_update
    }
    return Field (default, **kwargs) 

def _boot_init_logging():
    log = logging.getLogger()
    log.handlers.clear()
    stderr = logging.StreamHandler()
    format_str = '%(levelname)-5s %(filename)-5s %(lineno)-4s%(message)s'
    stderr.setFormatter(logging.Formatter(format_str))
    log.addHandler(stderr)
    log.setLevel(logging.DEBUG)
_boot_init_logging()
class _BootException(Exception):
    pass

async def _boot_stream_writing(worker: Worker):
    try:
        streams = await get_standard_streams()
    except Exception as e:
        raise _BootException(e)
    queue: asyncio.Queue[JsonDict] = asyncio.Queue()
    async def _flusher():
        while True:
            data = await queue.get()
            try:
                await worker.on_msg(data)
            except Exception as e:
                worker.log.error(f"Error while calling on_msg(): {e.__class__.__name__}:{e}")
            queue.task_done()
    async def _impl():
        r: asyncio.StreamReader = streams[0]
        w: asyncio.StreamWriter = streams[1]
        async def wr(msg: JsonDict):
            logging.getLogger().info("Flushing")
            w.write(msg.as_bytes())
            w.write(b"\r\n")
            await w.drain()
            logging.getLogger().info("Flushiied")
        worker.msgs.receive_with(wr)
        buf = str()
        while True:
            buf += (await r.read(100)).decode("utf-8").replace("'", '"')
            current_end = 0
            open_count = 0
            close_count = 0
            start_pos = 0
            for i, ch in enumerate(buf):
                if ch == '{':
                    if not open_count: start_pos = i
                    open_count+=1
                elif ch == '}':
                    close_count+=1
                if open_count and open_count == close_count:
                    current_obj = buf[start_pos:i - start_pos + 1]
                    try:
                        attempt = JsonDict(json.loads(current_obj))
                        queue.put_nowait(attempt)
                    except Exception as e:
                        worker.log.error(f"Error parsing Json: {e.__class__.__name__}:{e}")
                    current_end = i
            buf = buf[current_end + 1:]
    asyncio.get_running_loop().create_task(_flusher())
    await _impl()

def _boot_exec(params: BootParams):
    import importlib.util
    sys.modules["bootstrap"] = sys.modules["__main__"]
    spec = importlib.util.spec_from_file_location(params.worker_name, params.file)
    module = importlib.util.module_from_spec(spec)  #type: ignore
    sys.modules[params.worker_name] = module
    spec.loader.exec_module(module)  #type: ignore
    def _err(*args, **kwargs):
        raise RuntimeError("Use of print() function is forbidden")
    module.print = _err
    if hasattr(module, "main"):
        assert callable(module.main)
        main_params_count = len(inspect.signature(module.main).parameters)
        if inspect.iscoroutinefunction(module.main):
            async def _wrap():
                try:
                    if main_params_count >= 1: await module.main(params)
                    else: module.main()
                except Exception as e:
                    raise _BootException(e)
            params.ioloop.create_task(_wrap())
        else:
            if main_params_count >= 1: module.main(params)
            else: module.main()
    if hasattr(module, "worker"):
        worker: 'Worker' = module.worker(params)
        assert inspect.iscoroutinefunction(worker.on_msg)
        params.ioloop.create_task(_boot_stream_writing(worker))
        worker.run()
    else:
        raise RuntimeWarning("Could not find 'worker = MyWorkerClass'")
    
    def custom_exception_handler(loop, context):
        loop.default_exception_handler(context)
        exception = context.get('exception')
        if isinstance(exception, _BootException):
            logging.getLogger().error("Critical error on boot")
            loop.stop()

    params.ioloop.set_exception_handler(custom_exception_handler)
    params.ioloop.run_forever()

def _boot_main():
    """
    Radapter Python Modules Bootstrapper
    """
    parser = ArgumentParser(
        prog='bootstrap.py',
        description=_boot_main.__doc__,
        epilog='Bereg foreva'
    )
    parser.add_argument(
        "--settings",
        help="Settings to pass",
        required=False,
        dest="settings"
        )
    parser.add_argument(
        "--file",
        help="Target file",
        required=True,
        dest="file"
        )
    parser.add_argument(
        "--name",
        help="Worker Name",
        required=True,
        dest="name"
        )
    args = parser.parse_args()
    params = BootParams(
        args.file,
        json.loads(args.settings) if args.settings else {},
        args.name,
        asyncio.get_event_loop()
    )
    _boot_exec(params)

if __name__ == "__main__":
    _boot_main()
