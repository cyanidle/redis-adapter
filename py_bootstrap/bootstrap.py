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
import signal
import sys
import pathlib
import os
import traceback
from aioconsole import get_standard_streams
from types import CodeType
from typing import (TYPE_CHECKING, AbstractSet, Any, 
                    Awaitable, Callable, ClassVar, Deque,
                      Dict, Generic, Iterator,
                        List, Mapping, MutableMapping,
                          Optional, Sequence, Tuple,
                            Type, TypeVar, Union, cast, get_args, get_origin, overload)
from typing_extensions import ParamSpec, TypeVarTuple, dataclass_transform, Self
import debugpy

from pydantic import BaseModel, Extra, Field, validate_model, validator
import pydantic
from pydantic.fields import Undefined
assert sys.version_info.major >= 3
assert sys.version_info.minor >= 9

__all__=("BootParams", "Signal", "JsonDict", "Timer", "Worker", "JsonState")
_T = TypeVar("_T")

@dataclass
class BootParams:
    settings: Optional[dict]
    worker_name: str
    ioloop: asyncio.AbstractEventLoop
@dataclass
class _boot_internal_params:
    file: str
    test_data: Optional[str]

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

class _SignalMeta(Generic[_T]):
    class _CbTypes(IntEnum):
        CORO = auto()
        CORO_FUNC = auto()
        PLAIN = auto()
    __slots__ = ("_cb", "_argless", "_type")
    def __init__(self, cb: UniversalOrArgless[_T]) -> None:
        self._cb = cb
        if inspect.iscoroutine(cb):
            self._type = _SignalMeta._CbTypes.CORO
            self._argless = True
            return
        if inspect.iscoroutinefunction(cb):
            self._type = _SignalMeta._CbTypes.CORO_FUNC
        elif callable(cb):
            self._type = _SignalMeta._CbTypes.PLAIN
        else:
            raise TypeError(f"Invalid type is passed as callback to Signal!")
        sign = inspect.signature(cb)
        self._argless = not sign.parameters
    async def invoke(self, *args):
        if self._argless:
            args = tuple()
        if self._type == _SignalMeta._CbTypes.CORO:
            if args:
                raise RuntimeError(f"Signal emit on coroutine: {self._cb} with args (Was already wrapped)")
            await self._cb # type: ignore
        elif self._type == _SignalMeta._CbTypes.CORO_FUNC:
            await self._cb(*args) # type: ignore
        else:
            self._cb(*args) # type: ignore
            
class Signal(Generic[_T]):
    __slots__ = ("__targets", )
    def __init__(self) -> None:
        self.__targets: Dict[UniversalOrArgless[_T], _SignalMeta[_T]] = {}
    @property
    def callbacks(self):
        return tuple(self.__targets.keys())
    async def emit(self, *args: _T):
        await asyncio.gather(*(cb.invoke(*args) for cb in self.__targets.values()))
    def receive_with(self, callback: UniversalOrArgless[_T]):
        self.__targets[callback] = _SignalMeta(callback)
    def remove(self, callback: UniversalOrArgless[_T]):
        if callback in self.__targets:
            del self.__targets[callback]
    def clear(self):
        self.__targets.clear()
    def __del__(self):
        self.clear()


class Timer:
    __slots__ = ['_signal', '_ioloop', '_interval', '_task', '_single_shot', '_start_time', '_active']
    def __init__(self, *, ioloop: Optional[asyncio.AbstractEventLoop] = None) -> None:
        self._signal: Signal[None] = Signal()
        self._interval: int = 0
        self._single_shot = False
        self._ioloop = ioloop or asyncio.get_running_loop()
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
    def clear_callbacks(self):
        self.timeout.clear()
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
        self.__tasks: List[asyncio.Task] = []
        self.__name = params.worker_name
        self.__logger = logging.getLogger(f"worker.{params.worker_name}")
        self.__ioloop = params.ioloop
        self.__params = params
        self.__jsons: Signal[JsonDict] = Signal()
        self.__was_shutdown: Signal['Worker'] = Signal()
        self._with_error = False
        self._sync_sender: Optional[Callable[[JsonDict], None]] = None
    @overload
    async def send(self, state: 'JsonDict') -> None: ...
    @overload
    async def send(self, prefix: str, state: 'JsonState') -> None: ...
    @overload
    async def send(self, prefix: str, state: 'JsonItem') -> None: ...
    @overload
    async def send(**kwargs) -> None: ...
    async def send(self, prefix = None, state = None, **kwargs): # type: ignore
        if isinstance(prefix, JsonDict):
            await self.msgs.emit(prefix)
        elif isinstance(state, JsonState):
            await self.msgs.emit(JsonDict({prefix: state.send()}))
        elif isinstance(state, get_args(JsonItem)):
            await self.msgs.emit(JsonDict({prefix: state}))
        elif kwargs:
            await self.msgs.emit(JsonDict(**kwargs))
        else: raise TypeError(f"Unsupported type in send({state}): {state.__class__.__name__}")
    def send_sync(self, msg: 'JsonDict'):
        if self._sync_sender is not None:
            self._sync_sender(msg)
        else:
            self.log.warn(f"Sync send not available!")
    def run(self):
        try:
            self.on_run()
        except Exception as e:
            raise _BootException(f"{e.__class__.__name__}:{e}")
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
    def worker_params(self):
        return self.__params
    @property
    def was_shutdown(self):
        return self.__was_shutdown
    @property
    def msgs(self):
        return self.__jsons
    @property
    def name(self):
        return self.__name
    def shutdown(self, reason: str = 'Not Given', *, with_error = False):
        self._with_error = with_error
        self.log.warn(f"Shutting down... Reason: {reason}")
        try:
            was_tasks = len(self.tasks)
            self.on_shutdown()
            if len(self.tasks) > was_tasks:
                self.log.warning(f"### Do not create tasks inside of on_shutdown() handler!")
        except Exception as e:
            self.log.error("While shutting down:")
            self.log.exception(e)
        for task in self.tasks:
            task.cancel()
        self.create_task(partial(self.__was_shutdown.emit, self))
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
        return self.__logger #type: ignore
        #return self.__WorkerLogAdapter(self.__logger, worker = self)
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
            return (f"{msg}", kwargs) if self.worker.name else (msg, kwargs)

JsonKey = Union[Sequence[str], str]
JsonItem = Union[str, None, int, float, list, dict, Any]
FlatDict = Dict[str, JsonItem]

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
    def is_in(self, target: Union[dict, list]):
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
    def set_in(self, target: Union[dict, list], value: Any):
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
    def get_from(self, target: Union[dict, list]):
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

class JsonDict(MutableMapping[str, JsonItem]):
    __slots__ = ["_dict"]
    def __init__(self, src_dict: Union[FlatDict, dict, None] = None, *, nest = True, **kwargs) -> None:
        if src_dict is None: 
            src_dict = kwargs
        if isinstance(src_dict, JsonDict):
            src_dict = src_dict.top
        if not isinstance(src_dict, dict):
            raise TypeError(f"Can only construct JsonDict from dict! Passed: {src_dict.__class__.__name__}({src_dict})")
        self._dict = src_dict
        if self._dict and nest:
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
    def __bool__(self):
        return bool(self._dict)
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
    def get(self, *keys: Union[JsonKey, 'JsonDictIterator', Tuple[JsonKey]], default:_T = None) -> Union[JsonItem, _T]:
        try:
            if isinstance(keys, Sequence):
                if not keys: raise ValueError("Empty JsonDict key!")
                return self.get(':'.join(keys)) #type: ignore
            elif isinstance(keys, JsonDictIterator):
                return self.get(keys.key())
            else:
                return self.__getitem__(keys)
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


class _TraverseState:
    __slots__ = ("iter", "key", "container", "count", "value")
    def __init__(self, iter, key, container, count, value) -> None:
        self.iter: Union[Iterator[str], Iterator[JsonItem]] = iter
        self.key: Optional[str] = key
        self.container: Union[dict, list] = container
        self.count: int = count
        self.value: JsonItem = value

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
                raise
            else:
                self.__pop_state()
                self.__next__()
        return self
    
class JsonDictIteratorSimple(JsonDictIterator):
    def __next__(self) -> str:
        super().__next__()
        return str(self)


def _fetch_field_info(name: str, namespaces: dict) -> Optional[pydantic.fields.FieldInfo]:
    if name in namespaces: 
        if isinstance(namespaces[name], pydantic.fields.FieldInfo):
            return namespaces[name]
        else:
            return Field(default=namespaces.get(name, pydantic.fields.Undefined))
    else:
        return None

@dataclass_transform(kw_only_default=True, field_specifiers=(Field,))
class _JsonStateMeta(pydantic.main.ModelMetaclass):
    def __new__(cls, name: str, bases: Tuple[type], namespaces: Dict[str, Any], **kwargs):
        if name == "JsonState":
            return super().__new__(cls, name, bases, namespaces, **kwargs)
        annotations: dict = namespaces.get('__annotations__', {})
        for base in bases:
            for base_ in base.__mro__:
                if base_ is BaseModel or base_ is JsonState:
                    break
                annotations.update(base_.__annotations__)
        for field, ann in annotations.items():
            if field.startswith('__'): continue
            field_info = _fetch_field_info(field, namespaces)
            stripped = get_origin(ann) or ann
            if stripped is Union:
                raise TypeError(f"Unions are not supported in JsonState!")
            if not issubclass(stripped, object):
                raise _BootException(f"JsonState cannot use fields annotated with non-types! Field: {field}. Annotation: {stripped}")
            # check if field default constructible
            if not issubclass(stripped, JsonState) and namespaces.get(field) is None:
                try:
                    stripped()
                except:
                    raise _BootException(f"Cannot infer default value for field '{field}' of type '{stripped}' inside of {name}. Please provide default value!")
            if field_info is not None:
                if field_info.default is pydantic.fields.Undefined and not field_info.default_factory:
                    namespaces[field].default_factory=stripped
            else:
                namespaces[field] = Field(default_factory=stripped)    
        namespaces['__annotations__'] = annotations
        return super().__new__(cls, name, bases, namespaces, **kwargs)

#if not TYPE_CHECKING:
class JsonState(BaseModel, metaclass=_JsonStateMeta, extra=Extra.allow):
    __slots__ = ("_after")
    def __init__(self, **data):
        super().__init__(**data)
        object.__setattr__(self, "_after", {})
        self._after: Dict[str, Signal]
    @classmethod
    def default(cls) -> Self: 
        return cls()
    async def update_with(self, data:Union[JsonDict, dict]) -> JsonDict: 
        if isinstance(data, JsonDict):
            return await self.__refresh(**data.top)
        elif isinstance(data, dict):
            return await self.__refresh(**data)
        else: raise TypeError("JsonState can obly be updated with dict or JsonDict")
    def send(self) -> JsonDict: 
        return JsonDict(self.dict())
    def after_update(self, part: _T, cb: UniversalOrArgless[_T]):
        for k, v in self.__dict__.items():
            if v is part:
                self.__sig(k).receive_with(cb)
                return
            elif isinstance(v, JsonState):
                try:
                    v.after_update(part, cb)
                    return
                except: 
                    pass
        raise ValueError(f"Field {part} does not belong to {self!r}. (Note: Nested fields inside containers are not supported for callbacks!)")
    def __sig(self, name: str):
        if not name in self._after: self._after[name] = Signal()
        return self._after[name]
    async def __handle_field(self, out: JsonDict, name: str, was, new):
        if isinstance(was, JsonState):
            out[name] = await was.__refresh(**new.dict(exclude_unset=True))
        else:
            logging.getLogger("bootstrap").info(f"Updating {name}: {was!r} --> {new!r}")
            setattr(self, name, new)
            out[name] = new
            sig = self._after.get(name)
            try:
                if sig is not None: await sig.emit(getattr(self, name))
            except Exception as e:
                logging.getLogger("bootstrap").error(f"{self}: While updating {name}:")
                logging.getLogger("bootstrap").exception(e)
    async def __refresh(self, **data) -> JsonDict:
        """Refresh the internal attributes with new data."""
        result = JsonDict()
        values, fields, error = validate_model(self.__class__, data)
        if error:
            logging.getLogger("bootstrap").exception(error)
            return JsonDict()
        for name in fields:
            if not hasattr(self, name): 
                logging.getLogger("bootstrap").warning(f"Extra property received --> {name}")
            value = values[name]
            was = getattr(self, name)
            await self.__handle_field(result, name, was, value)
        return result
# else:
#     class JsonState(metaclass=_JsonStateMeta):
#         @classmethod
#         def default(cls) -> Self: ...
#         # True means was updated at least one field(nested or not)
#         async def update_with(self, data: JsonDict) -> JsonDict: ...
#         def send(self) -> JsonDict: ...
#         def after_update(self, part: _T, cb: UniversalOrArgless[_T]): ...

class Weekday(IntEnum):
    MONDAY = 1
    TUESDAY = 2
    WEDNESDAY = 3
    THURSDAY = 4
    FRIDAY = 5
    SATURDAY = 6
    SUNDAY = 7
Hour24 = partial(Field, ge=0, le=24)
Minute = partial(Field, ge=0, le=60)

def _boot_init_logging():
    def _apply(log: logging.Logger, format: str, level):
        log.handlers.clear()
        stderr = logging.StreamHandler()
        format_str = format
        stderr.setFormatter(logging.Formatter(format_str))
        log.addHandler(stderr)
        log.setLevel(level)
    _apply(logging.getLogger("bootstrap"), '%(levelname)-5s %(message)s', logging.INFO)
    _apply(logging.getLogger("worker"), '%(levelname)-5s %(filename)-5s %(lineno)-4s%(message)s', logging.DEBUG)

class _BootException(Exception):
    pass

async def _connect_to_worker(worker: Worker):
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
    def _sync_send(msg: JsonDict):
        print(msg.as_bytes() + b"\r\n")
    worker._sync_sender = _sync_send
    w: asyncio.StreamWriter = streams[1]
    async def wr(msg: JsonDict):
        w.write(msg.as_bytes() + b"\r\n")
        await w.drain()
    worker.msgs.receive_with(wr)
    async def _impl():
        logging.getLogger("bootstrap").info("Started read")
        r: asyncio.StreamReader = streams[0]
        sleep = 1
        max_sleep = 5
        while True:
            if r.at_eof(): 
                logging.getLogger("bootstrap").error(f"Stdin EOF!")
                await asyncio.sleep(sleep)
                if sleep < max_sleep: sleep += 1
                continue
            buf = await r.readline()
            try:
                attempt = JsonDict(json.loads(buf.decode("utf-8")))
                queue.put_nowait(attempt)
            except Exception as e:
                worker.log.error(f"Error parsing Json: {e.__class__.__name__}:{e}")
    worker.run()
    logging.getLogger("bootstrap").info(f"Connected stdin/stdout to worker!")
    await asyncio.gather(_impl(), _flusher())

async def _watchdog():
    while True:
        await asyncio.sleep(10)

async def _boot_test(worker: Worker, json: JsonDict):
    await asyncio.sleep(2)
    await worker.on_msg(json)

def _boot_exec(params: BootParams, internal: _boot_internal_params):
    import importlib.util
    sys.modules["bootstrap"] = sys.modules["__main__"]
    spec = importlib.util.spec_from_file_location(params.worker_name, internal.file)
    module = importlib.util.module_from_spec(spec)  #type: ignore
    sys.modules[params.worker_name] = module
    spec.loader.exec_module(module)  #type: ignore
    def _err(*args, **kwargs):
        raise RuntimeError("Use of print() function is forbidden")
    module.print = _err #type: ignore
    if hasattr(module, "main"):
        assert callable(module.main)
        main_params_count = len(inspect.signature(module.main).parameters)
        if inspect.iscoroutinefunction(module.main):
            async def _wrap():
                try:
                    if main_params_count >= 1: await module.main(params)
                    else: await module.main()
                except Exception as e:
                    raise _BootException(e)
            params.ioloop.create_task(_wrap())
        else:
            if main_params_count >= 1: module.main(params)
            else: module.main()
    if hasattr(module, "worker"):
        worker: 'Worker' = module.worker(params)
        assert inspect.iscoroutinefunction(worker.on_msg)
        params.ioloop.create_task(_connect_to_worker(worker))
    else:
        raise RuntimeWarning("Could not find 'worker = MyWorkerClass'")
    
    def custom_exception_handler(loop, context):
        loop.default_exception_handler(context)
        exception = context.get('exception')
        if isinstance(exception, _BootException):
            logging.getLogger("bootstrap").error("Critical error on boot")
            loop.stop()
            sys.exit(-1)
    params.ioloop.set_exception_handler(custom_exception_handler)
    async def _was_shutdown():
        logging.getLogger("bootstrap").warning(f"Worker is shutting down! Waiting 5 seconds before quitting")
        await asyncio.sleep(5)
        params.ioloop.stop()
        params.ioloop.close()
        if worker._with_error:
            sys.exit(-1)
        else:
            sys.exit(0)
    worker.was_shutdown.receive_with(_was_shutdown)
    if not internal.test_data is None:
        as_json = JsonDict(json.loads(internal.test_data))
        params.ioloop.create_task(_boot_test(worker, as_json))
    if not sys.platform.startswith("win32"):
        params.ioloop.add_signal_handler(signal.SIGTERM, worker.shutdown)  
        params.ioloop.add_signal_handler(signal.SIGQUIT, worker.shutdown)  
    params.ioloop.create_task(_watchdog())
    params.ioloop.run_forever()

def _boot_main():
    """
    Radapter Python Modules Bootstrapper
    """
    _boot_init_logging()
    parser = ArgumentParser (
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
    parser.add_argument(
        "--wait_for_debug_client",
        help="Before start wait for debugpy clients",
        required=False,
        dest="wait_for_debug_client",
        type=bool
        )
    parser.add_argument(
        "--debug_port",
        help="Listen for debugpy clients",
        required=False,
        dest="debug_port",
        type=int
        )
    parser.add_argument(
        "--test_data",
        help="Test data to pass to module",
        required=False,
        dest="test_data"
        )
    args = parser.parse_args()
    params = BootParams(
        json.loads(args.settings) if args.settings else {},
        args.name,
        asyncio.get_event_loop()
    )
    internal = _boot_internal_params(
        args.file,
        args.test_data
    )
    if args.debug_port is not None:
        logging.getLogger("bootstrap").warning(f"Accepting clients to connect to debugging port: {args.debug_port}")
        debugpy.listen(("0.0.0.0", args.debug_port))
        if args.wait_for_debug_client:
            logging.getLogger("bootstrap").warning("Waiting for connection!")
            debugpy.wait_for_client()
            logging.getLogger("bootstrap").warning(f"Debug client attached!")
    logging.getLogger("bootstrap").info("Starting boot sequence!")
    _boot_exec(params, internal)

if __name__ == "__main__":
    _boot_main()
