import time
import json
import os
import pytest
from datetime import datetime, timedelta

from tests import *


def _timestamp():
    return time.time_ns() / 1000_000.0


async def basic_test(type: str, val, rval=None):
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1",
            # We are only allowed string expression in payload!
            "payload": isinstance(val, str) and val or json.dumps(val),
            "payloadType": type, "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    if rval != None:
        assert msgs[0]["payload"] == rval
    else:
        assert msgs[0]["payload"] == val


@pytest.mark.asyncio
async def test_0001():
    '''should works with simple payload'''
    await basic_test("num", 10)
    await basic_test("str", "10")
    await basic_test("bool", True)
    val_json = '{ "x":"vx", "y":"vy", "z":"vz" }'
    await basic_test("json", val_json, json.loads(val_json))
    val_buf = '[1,2,3,4,5]'
    await basic_test("bin", val_buf, json.loads(val_buf))


@pytest.mark.asyncio
async def test_0002():
    '''inject value of environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_TEST", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    os.environ["NR_TEST"] = "foo"
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    del os.environ["NR_TEST"]
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "foo"


@pytest.mark.asyncio
async def test_0003():
    '''inject name of node as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_NODE_NAME", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "NAME"


@pytest.mark.asyncio
async def test_0004():
    '''inject id of node as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_NODE_ID", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "0000000000000001"


"""
@pytest.mark.asyncio
async def test_0005():
    '''inject path of node as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_NODE_PATH", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "100/1"  # TODO FIXME
"""


@pytest.mark.asyncio
async def test_0006():
    '''inject name of flow as environment variable'''
    flows = [
        {"id": "100", "type": "tab", "label": "FLOW"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_FLOW_NAME", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "FLOW"


@pytest.mark.asyncio
async def test_0007():
    '''inject id of flow as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
            "topic": "t1", "payload": "NR_FLOW_ID", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "0000000000000100"


@pytest.mark.asyncio
async def test_0008():
    '''inject name of group as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
         "g": "FF", "topic": "t1", "payload": "NR_GROUP_NAME", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
        {"id": "FF", "z": "100", "type": "group", "name": "GROUP"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "GROUP"


@pytest.mark.asyncio
async def test_0009():
    '''inject id of group as environment variable'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
         "g": "FF", "topic": "t1", "payload": "NR_GROUP_ID", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
        {"id": "FF", "z": "100", "type": "group", "name": "GROUP"}
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "00000000000000ff"


@pytest.mark.asyncio
async def test_0010():
    '''inject name of node as environment variable by substitution'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
         "topic": "t1", "payload": r"${NR_NODE_NAME}", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "NAME"


@pytest.mark.asyncio
async def test_0011():
    '''inject id of node as environment variable by substitution'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "0000000000000001", "z": "100", "type": "inject", "name": "NAME",
         "once": True, "onceDelay": 0.0, "repeat": "",
         "topic": "t1", "payload": r"${NR_NODE_ID}", "payloadType": "env", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "0000000000000001"


"""
@pytest.mark.asyncio
async def test_0101():
    '''sets the value of flow context property'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "name": "NAME", "once": True, "onceDelay": 0.0, "repeat": "",
         "topic": "t1", "payload": "flowValue", "payloadType": "flow", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] == "changeMe"
"""


@pytest.mark.asyncio
async def test_0201():
    '''should inject once with default delay property'''
    # Since we cannot got the property in the node
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True,
         "topic": "t1", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'


@pytest.mark.asyncio
async def test_0202():
    '''should inject once with default delay'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True,
         "topic": "t1", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    # Should in one second
    expected_time = _timestamp() + 1000.0
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]['payload'] < expected_time


@pytest.mark.asyncio
async def test_0203():
    '''should inject once with 500 msec. delay'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True, "onceDelay": 0.5,
         "topic": "t1", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    start_time = _timestamp()
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] >= start_time + 500.0
    assert msgs[0]["payload"] < start_time + 1000.0  # in 1-second


@pytest.mark.asyncio
async def test_0204():
    '''should inject once with delay of two seconds'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "once": True, "onceDelay": 2,
         "topic": "t1", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    start_time = _timestamp()
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] >= start_time + 2000.0
    assert msgs[0]["payload"] < start_time + 2700.0


@pytest.mark.asyncio
async def test_0205():
    '''should inject repeatedly'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "repeat": 0.2,
         "topic": "t2", "payload": "payload", "payloadType": "str", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 2)
    assert msgs[0]["topic"] == 't2'
    assert msgs[0]["payload"] == "payload"
    assert msgs[1]["topic"] == 't2'
    assert msgs[1]["payload"] == "payload"


@pytest.mark.asyncio
async def test_0206():
    '''should inject once with delay of two seconds and repeatedly'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject",
         "once": True, "onceDelay": 1.2, "repeat": 0.2,
         "topic": "t1", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    start_time = _timestamp()
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 2)
    assert msgs[0]["topic"] == 't1'
    assert msgs[0]["payload"] > start_time + 1000.0


@pytest.mark.asyncio
async def test_0207():
    '''should inject with cron'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {"id": "1", "z": "100", "type": "inject", "crontab": "* * * * * *",
         "topic": "t3", "payload": "", "payloadType": "date", "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    start_time = _timestamp()
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    assert msgs[0]["topic"] == 't3'
    payload = msgs[0]["payload"]
    assert isinstance(payload, float) or isinstance(payload, int)
    assert payload > start_time


@pytest.mark.asyncio
async def test_0208():
    '''should inject multiple properties'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {
            "id": "1",
            "type": "inject",
            "z": "100",
            "once": True,
            "props": [
                {"p": "topic", "v": "t1", "vt": "str"},
                {"p": "payload", "v": "foo", "vt": "str"},
                {"p": "x", "v": "10", "vt": "num"},
                # {"p": "y", "v": "x+2", "vt": "jsonata"} #TODO FIXME
            ],
            "wires": [["2"]],
        },
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    msg = msgs[0]
    assert msg["topic"] == "t1"
    assert msg["payload"] == "foo"
    assert msg["x"] == 10
    # assert msg["y"] == 12


"""
# EdgeLink doesn't support the msg injection for `inject` node
@pytest.mark.asyncio
async def test_0209():
    '''should inject custom properties in message'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {
            "id": "1",
            "type": "inject",
            "z": "100",
            "once": True,
            "props": [
                {"p": "payload", "v": "static", "vt": "str"},
                {"p": "topic", "v": "static", "vt": "str"},
                {"p": "bool1", "v": "true", "vt": "bool"}
            ],
            "wires": [["2"]],
        },
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = [
        'nid': '1',
        'msg': {
            '__user_inject_props__': {
                {p:"topic", v:"t_override", vt:"str"}, //change value to t_override
                {p:"str1", v:"1", vt:"num"}, //change type
                {p:"num1", v:"1", vt:"num"}, //new prop
                {p:"bool1", v:"false", vt:"bool"}, //change value to false
            }
        }
    ]
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    msg = msgs[0]
    assert msg["topic"] == "t1"
    assert msg["payload"] == "foo"
    assert msg["x"] == 10
    # assert msg["y"] == 12
"""


@pytest.mark.asyncio
async def test_0210():
    '''should inject multiple properties using legacy props if needed'''
    flows = [
        {"id": "100", "type": "tab"},  # flow 1
        {
            "id": "1",
            "type": "inject",
            "payload": "123",
            "payloadType": "num",
            "topic": "foo",
            "once": True,
            "props": [
                {"p": "topic", "vt": "str"},
                {"p": "payload"}
            ],
            "wires": [["2"]],
            "z": "100"
        },
        {"id": "2", "z": "100", "type": "console-json"},
    ]
    injections = []
    msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
    msg = msgs[0]
    assert msg["topic"] == "foo"
    assert msg["payload"] == 123


# 0211: should report invalid JSONata expression
# We don't support JSONata yet...