from asyncio import sleep
import sys
from bootstrap import BootParams, Worker, JsonDict

class TestWorker(Worker):
    def __init__(self, params: BootParams) -> None:
        super().__init__(params)
    async def on_msg(self, msg: JsonDict):
        self.log.info(msg.top)
    def on_shutdown(self) -> None:
        pass
    def on_run(self) -> None:
        self.create_task(self.spin)
    async def spin(self):
        while True:
            #await self.msgs.emit(JsonDict({"a": 1}))
            await sleep(1)
            #self.log.info("OK")
            await sleep(1)

worker = TestWorker