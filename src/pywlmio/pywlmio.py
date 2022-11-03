import asyncio
from collections import namedtuple
import enum
import os
from struct import Struct, pack, unpack
from typing import Callable, Iterable
from _wlmio import *

__all__ = []


Status = namedtuple("Status", ["uptime", "health", "mode", "vendor_status"])

status_callbacks = [set() for i in range(129)]

def register_status_callback(id: int, callback: Callable) -> None:
  assert id is None or (id >= 0 and id < 128)
  assert asyncio.iscoroutinefunction(callback)
  if id is None:
    status_callbacks[128].add(callback)
  else:
    status_callbacks[id].add(callback)
__all__.append("register_status_callback")

def unregister_status_callback(id: int, callback: Callable) -> None:
  assert id is None or (id >= 0 and id < 128)
  if id is None:
    status_callbacks[128].discard(callback)
  else:
    status_callbacks[id].discard(callback)
__all__.append("unregister_status_callback")

def status_callback(node_id: int, old_status: bytes, new_status: bytes) -> None:
  old_status = Status._make(unpack("IBBB0L", old_status))
  new_status = Status._make(unpack("IBBB0L", new_status))

  for callback in status_callbacks[node_id]:
    asyncio.create_task(callback(old_status, new_status))

  for callback in status_callbacks[128]:
    asyncio.create_task(callback(node_id, old_status, new_status))

set_status_callback(status_callback)


def init() -> None:
    # raise RuntimeError("pywlmio already initialised")
  asyncio.get_running_loop().add_reader(get_epoll_fd(), tick)
__all__.append("init")


Version = namedtuple("Version", ["major", "minor"])
NodeInfo = namedtuple(
  "NodeInfo",
  [
    "protocol_version",
    "hardware_version",
    "software_version",
    "software_vcs_revision_id",
    "unique_id",
    "name",
    "software_image_crc",
    "certificate_of_authenticity"
  ]
)


__all__.append("RegisterType")
@enum.unique
class RegisterType(enum.IntEnum):
  EMPTY = 0
  STRING = 1
  UNSTRUCTURED = 2
  BIT = 3
  INT64 = 4
  INT32 = 5
  INT16 = 6
  INT8 = 7
  UINT64 = 8
  UINT32 = 9
  UINT16 = 10
  UINT8 = 11
  FLOAT64 = 12
  FLOAT32 = 13
  FLOAT16 = 14


__all__.append("WlmioError")
class WlmioError(Exception):
  pass


__all__.append("WlmioWrongNodeError")
class WlmioWrongNodeError(WlmioError):
  def __init__(self, expected: str, actual: str):
    self.expected = expected
    self.actual = actual


__all__.append("WlmioInternalError")
class WlmioInternalError(WlmioError):
  def __init__(self, e: int):
    self.errno = e
    self.errstr = os.strerror(e)


__all__.append("Node")
class Node:
  def __init__(self, id: int):
    assert id >= 0 and id < 128
    self.id = id
    self.status = None
    self.info = None
    register_status_callback(id, self._status_callback)

    self.lock = asyncio.Lock()

  def __del__(self):
    unregister_status_callback(id, self._status_callback)

  async def get_info(self) -> NodeInfo:
    async with self.lock:
      if self.info is None:
        loop = asyncio.get_running_loop()

        r = await get_node_info(self.id, loop.create_future())
        if r[0] < 0:
          raise WlmioInternalError(-r[0])

        s = Struct("6BQ16s51sQ222s0L")
        temp = s.unpack(r[1][0:s.size])

        pv = Version._make(temp[0:2])
        hv = Version._make(temp[2:4])
        sv = Version._make(temp[4:6])
        self.info = NodeInfo(pv, hv, sv, temp[6], temp[7], temp[8].decode("utf-8").rstrip("\0"), temp[9], None)

      return self.info

  async def _status_callback(self, old_status: Status, new_status: Status) -> None:
    if(self.status is None):
      self.status = old_status

    # node went offline
    if(new_status.mode == 7):
      self.info = None

    # node came online or restarted
    if((old_status.mode == 7 and new_status.mode != 7) or new_status.uptime < old_status.uptime):
      self.info = None

    self.status = new_status

  def is_online(self) -> bool:
    return self.status is not None and self.status.mode != 7

  async def register_access(self, name: str, reg_type: RegisterType, value: Iterable):
    loop = asyncio.get_running_loop()

    s = Struct("BH256s")
    length = 0
    data = bytes()

    if reg_type is None or value is None:
      reg_type = RegisterType.EMPTY
    elif reg_type == RegisterType.UINT32:
      length = len(value)
      data = pack("{0}L".format(length), *value)
    elif reg_type == RegisterType.UINT16:
      length = len(value)
      data = pack("{0}H".format(length), *value)
    elif reg_type == RegisterType.UINT8:
      length = len(value)
      data = pack("{0}B".format(length), *value)

    regw = s.pack(reg_type, length, data)
    regr = await register_access(self.id, name, regw, loop.create_future())
    if regr[0] < 0:
      raise WlmioInternalError(-regr[0])
    regr = s.unpack(regr[1])

    if regr[0] == RegisterType.UINT32:
      regr = (regr[0], unpack("{0}L".format(regr[1]), regr[2][0:regr[1]*4]))
    if regr[0] == RegisterType.UINT16:
      regr = (regr[0], unpack("{0}H".format(regr[1]), regr[2][0:regr[1]*2]))
    if regr[0] == RegisterType.UINT8:
      regr = (regr[0], unpack("{0}B".format(regr[1]), regr[2][0:regr[1]]))

    return regr

  async def list_registers(self):
    loop = asyncio.get_running_loop()

    names = []
    for i in range(2 ** 16 - 1):
      r = await register_list(self.id, i, loop.create_future())
      if len(r[1]) == 0:
        break

      names.append(r[1])

    return names


  async def reboot(self, delay=None):
    future = asyncio.get_running_loop().create_future()

    if delay is None:
      return await execute_command(self.id, 65535, bytes(), future)
    else:
      return await execute_command(self.id, 65535, pack("H", delay), future)


class IoChannel:
  def __init__(self, node: Node, name: str):
    self.node = node
    self.name = name

  async def _check_name(self):
    info = await self.node.get_info()
    if info.name != self.name:
      raise WlmioWrongNodeError(self.name, info.name)

  async def write(self):
    await self._check_name()

  async def read(self):
    await self._check_name()

  async def configure(self):
    await self._check_name()


class VPE6010Channel(IoChannel):
  def __init__(self, node: Node):
    super().__init__(node, "com.widgetlords.mio.6010")

  async def read(self):
    await super().read()
    r = await self.node.register_access("input", RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1]


__all__.append("VPE6010")
class VPE6010(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6010Channel(self)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])


class VPE6030Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6030")
    self.reg = "ch{}.output".format(ch)

  async def write(self, value):
    assert value == 0 or value == 1
    await super().write()
    await self.node.register_access(self.reg, RegisterType.UINT8, [value])

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT8
    return r[1][0]


__all__.append("VPE6030")
class VPE6030(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6030Channel(self, 1)
    self.ch2 = VPE6030Channel(self, 2)
    self.ch3 = VPE6030Channel(self, 3)
    self.ch4 = VPE6030Channel(self, 4)


class VPE6040Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6040")
    self.reg = "ch{}.input".format(ch)
    self.mode_reg = "ch{}.mode".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]

  async def configure(self, mode: int):
    assert mode >= 0 and mode <= 2
    await super().configure()
    await self.node.register_access(self.mode_reg, RegisterType.UINT8, [mode])


__all__.append("VPE6040")
class VPE6040(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6040Channel(self, 1)
    self.ch2 = VPE6040Channel(self, 2)
    self.ch3 = VPE6040Channel(self, 3)
    self.ch4 = VPE6040Channel(self, 4)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])


class VPE6050Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6050")
    self.reg = "ch{}.output".format(ch)
    self.mode_reg = "ch{}.mode".format(ch)

  async def write(self, value):
    await super().write()
    await self.node.register_access(self.reg, RegisterType.UINT16, [value])

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]

  async def configure(self, mode: int):
    assert mode == 0 or mode == 1
    await super().configure()
    await self.node.register_access(self.mode_reg, RegisterType.UINT8, [mode])


__all__.append("VPE6050")
class VPE6050(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6050Channel(self, 1)
    self.ch2 = VPE6050Channel(self, 2)
    self.ch3 = VPE6050Channel(self, 3)
    self.ch4 = VPE6050Channel(self, 4)


class VPE6060Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6060")
    self.reg = "ch{}.input".format(ch)
    self.bias_reg = "ch{}.bias".format(ch)
    self.polarity_reg = "ch{}.polarity".format(ch)
    self.mode_reg = "ch{}.mode".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT32
    return r[1][0]

  async def configure(self, mode: int, polarity: int, bias: int):
    assert mode >= 0 and mode <= 2
    assert polarity == 0 or polarity == 1
    assert bias >= 0 and bias <= 2
    await super().configure()
    await asyncio.gather(
      self.node.register_access(self.bias_reg, RegisterType.UINT8, [bias]),
      self.node.register_access(self.polarity_reg, RegisterType.UINT8, [polarity]),
      self.node.register_access(self.mode_reg, RegisterType.UINT8, [mode])
    )


__all__.append("VPE6060")
class VPE6060(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6060Channel(self, 1)
    self.ch2 = VPE6060Channel(self, 2)
    self.ch3 = VPE6060Channel(self, 3)
    self.ch4 = VPE6060Channel(self, 4)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])


class VPE6070Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6070")
    self.reg = "ch{}.output".format(ch)

  async def write(self, value):
    await super().write()
    await self.node.register_access(self.reg, RegisterType.UINT16, [value])

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]


__all__.append("VPE6070")
class VPE6070(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6070Channel(self, 1)
    self.ch2 = VPE6070Channel(self, 2)
    self.ch3 = VPE6070Channel(self, 3)
    self.ch4 = VPE6070Channel(self, 4)


class VPE6080Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6080")
    self.reg = "ch{}.input".format(ch)
    self.enabled_reg = "ch{}.enabled".format(ch)
    self.beta_reg = "ch{}.beta".format(ch)
    self.t0_reg = "ch{}.t0".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]

  async def configure(self, enabled: int, beta: int = 3380, t0: float = 298.15):
    assert enabled == 0 or enabled == 1
    await super().configure()
    await asyncio.gather(
      self.node.register_access(self.enabled_reg, RegisterType.UINT8, [enabled]),
      self.node.register_access(self.beta_reg, RegisterType.UINT16, [beta]),
      self.node.register_access(self.t0_reg, RegisterType.UINT16, [int(t0 * 100)])
    )


__all__.append("VPE6080")
class VPE6080(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6080Channel(self, 1)
    self.ch2 = VPE6080Channel(self, 2)
    self.ch3 = VPE6080Channel(self, 3)
    self.ch4 = VPE6080Channel(self, 4)
    self.ch5 = VPE6080Channel(self, 5)
    self.ch6 = VPE6080Channel(self, 6)
    self.ch7 = VPE6080Channel(self, 7)
    self.ch8 = VPE6080Channel(self, 8)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])


class VPE6090Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6090")
    self.reg = "ch{}.input".format(ch)
    self.type_reg = "ch{}.type".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]

  async def configure(self, t: int):
    await super().configure()
    await self.node.register_access(self.type_reg, RegisterType.UINT8, [t])


__all__.append("VPE6090")
class VPE6090(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6090Channel(self, 1)
    self.ch2 = VPE6090Channel(self, 2)
    self.ch3 = VPE6090Channel(self, 3)
    self.ch4 = VPE6090Channel(self, 4)
    self.ch5 = VPE6090Channel(self, 5)
    self.ch6 = VPE6090Channel(self, 6)

  async def configure(self):
    await self.ch1._check_name()


class VPE6180Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6180")
    self.reg = "ch{}.input".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT16
    return r[1][0]


__all__.append("VPE6180")
class VPE6180(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6180Channel(self, 1)
    self.ch2 = VPE6180Channel(self, 2)
    self.ch3 = VPE6180Channel(self, 3)
    self.ch4 = VPE6180Channel(self, 4)
    self.ch5 = VPE6180Channel(self, 5)
    self.ch6 = VPE6180Channel(self, 6)
    self.ch7 = VPE6180Channel(self, 7)
    self.ch8 = VPE6180Channel(self, 8)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])


class VPE6190Channel(IoChannel):
  def __init__(self, node: Node, ch: int):
    super().__init__(node, "com.widgetlords.mio.6190")
    self.reg = "ch{}.input".format(ch)
    self.enabled_reg = "ch{}.enabled".format(ch)

  async def read(self):
    await super().read()
    r = await self.node.register_access(self.reg, RegisterType.EMPTY, None)
    assert r[0] == RegisterType.UINT32
    return r[1][0]

  async def configure(self, enable: int):
    await super().configure()
    await self.node.register_access(self.enabled_reg, RegisterType.UINT8, [enable])


__all__.append("VPE6190")
class VPE6190(Node):
  def __init__(self, id: int):
    super().__init__(id)

    self.ch1 = VPE6190Channel(self, 1)
    self.ch2 = VPE6190Channel(self, 2)
    self.ch3 = VPE6190Channel(self, 3)

  async def configure(self, sample_interval: int):
    await self.ch1._check_name()
    await self.register_access("sample_interval", RegisterType.UINT16, [sample_interval])