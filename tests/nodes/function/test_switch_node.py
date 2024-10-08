import json
import pytest
import pytest_asyncio
import asyncio
import os

from tests import *

async def _generic_switch_test(rule, rule_with, checkall, should_receive, send_payload):
    flow = [
        {"id": "100", "type": "tab"},
        {
            "id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload",
            "checkall": checkall, "outputs": 1, "wires": [["2"]],
            "rules": [
                {"t": rule, "v": rule_with, "vt": isinstance(rule_with, str) and "str" or "num"} #FIXME
            ],
        },
        {"id": "2", "z": "100", "type": "test-once"}
    ]
    await _custom_flow_switch_test(flow, should_receive, send_payload)

async def _custom_flow_switch_test(flow, should_receive, send_payload):
    await _custom_flow_message_switch_test(flow, should_receive, {"payload": send_payload})

async def _custom_flow_message_switch_test(flow, should_receive, message):
    injections = [
        {"nid": "1", "msg": message},
    ]
    nexpected = should_receive and 1 or 0
    msgs = await run_flow_with_msgs_ntimes(flows_obj=flow, msgs=injections, nexpected=nexpected, timeout=0.2)
    if should_receive:
        assert msgs[0]["payload"] == message["payload"]


@pytest.mark.describe('switch Node')
class TestSwitchNode:

    @pytest.mark.asyncio
    @pytest.mark.it('should check if payload equals given value')
    async def test_it_should_check_if_payload_equals_given_value(self):
        await _generic_switch_test("eq", "Hello", True, True, "Hello")
