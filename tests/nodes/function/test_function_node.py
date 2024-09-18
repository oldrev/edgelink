import pytest
import os

from tests import *

# 0001 should do something with the catch node

@pytest.mark.describe('function Node')
class TestFunctionNode:

    @pytest.mark.asyncio
    @pytest.mark.it('''should send returned message''')
    async def test_0002(self):
        node = {
            "type": "function",
            "func": "return msg;",
        }
        msgs = await run_with_single_node_ntimes('str', 'foo', node, 1, once=True, topic='bar')
        assert msgs[0]['topic'] == 'bar'
        assert msgs[0]['payload'] == 'foo'

    @pytest.mark.asyncio
    @pytest.mark.it('should send returned message using send()')
    async def test_it_should_send_returned_message_using_send_1(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [["2"]], "func": "node.send(msg);"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar'}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["topic"] == "bar"
        assert msgs[0]["payload"] == "foo"

    @pytest.mark.asyncio
    @pytest.mark.it('should allow accessing node.id and node.name and node.outputCount')
    async def test_it_should_allow_accessing_node_id_and_node_name_and_node_output_count(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "name": "test-function", "wires": [["2"]], "outputs": 2,
                "func": "return [{ topic: node.name, payload:node.id, outputCount: node.outputCount }];",
             },
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': ''}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["payload"] == "0000000000000001"
        assert msgs[0]["topic"] == "test-function"
        assert msgs[0]["outputCount"] == 2

    async def _test_send_cloning(self, args):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [["2"], ["2"]],
                "func": f"node.send({args}); msg.payload = 'changed';"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar'}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["topic"] == "bar"
        assert msgs[0]["payload"] == "foo"

    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it('should clone single message sent using send()')
    async def test_it_should_clone_single_message_sent_using_send_2(self):
        await self._test_send_cloning("msg")


    # Not supported, yet
    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it('should not clone single message sent using send(,false)')
    async def test_it_should_not_clone_single_message_sent_using_send_false(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [
                ["2"]], "func": "node.send(msg,false); msg.payload = 'changed';"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar'}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["topic"] == "bar"
        assert msgs[0]["payload"] == "changed"

    @pytest.mark.asyncio
    @pytest.mark.it('should clone first message sent using send() - array 1')
    async def test_it_should_clone_first_message_sent_using_send_array_1(self):
        await self._test_send_cloning("[msg]")

    @pytest.mark.asyncio
    @pytest.mark.it('should clone first message sent using send() - array 2')
    async def test_it_should_clone_first_message_sent_using_send_array_2(self):
        await self._test_send_cloning("[[msg],[null]]")

    @pytest.mark.asyncio
    @pytest.mark.it('should clone first message sent using send() - array 3')
    async def test_it_should_clone_first_message_sent_using_send_array_3(self):
        await self._test_send_cloning("[null,msg]")

    @pytest.mark.asyncio
    @pytest.mark.it('should clone first message sent using send() - array 3')
    async def test_it_should_clone_first_message_sent_using_send_array_3_1(self):
        await self._test_send_cloning("[null,[msg]]")

    @pytest.mark.asyncio
    @pytest.mark.it('should pass through _topic')
    async def test_it_should_pass_through__topic(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [["2"]], "func": "return msg;"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar', '_topic': 'barz'}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["topic"] == "bar"
        assert msgs[0]["payload"] == "foo"
        assert msgs[0]["_topic"] == "barz"

    @pytest.mark.asyncio
    @pytest.mark.it('should send to multiple outputs')
    async def test_it_should_send_to_multiple_outputs(self):
        node = {
            "type": "function",
            "func": "var msg2 = RED.util.cloneMessage(msg); msg2.payload='p2'; return [msg, msg2];",
            "wires": [["3"], ["3"]]
        }
        msgs = await run_with_single_node_ntimes('str', 'foo', node, 2, once=True, topic='bar')
        assert msgs[0]['topic'] == 'bar'
        assert msgs[0]['topic'] == msgs[1]['topic']
        assert msgs[0]['payload'] != msgs[1]['payload']
        assert sorted([msgs[0]['payload'], msgs[1]['payload']]) == ['foo', 'p2']

    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it('should send to multiple messages')
    async def test_it_should_send_to_multiple_message(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [["2"]], "func": "return [[{payload: 1},{payload: 2}]];"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar', '_msgid': 1234}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 2)
        assert msgs[0]['topic'] == msgs[1]['topic'] == 'bar'
        assert msgs[0]['_msgid'] == msgs[1]['_msgid'] == 1234
        assert msgs[0]['payload'] == 1
        assert msgs[1]['payload'] == 2
        assert sorted([msgs[0]['payload'], msgs[1]['payload']]) == ['foo', 'p2']


    # TODO the testing frame has no way to handle time-out for now
    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it('should allow input to be discarded by returning null')
    async def test_it_should_allow_input_to_be_discarded_by_returning_null(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "function", "z": "100", "wires": [["2"]], "func": "return null;"},
            {"id": "2", "z": "100", "type": "console-json"}
        ]
        injections = [
            {"nid": "1", "msg": {'payload': 'foo', 'topic': 'bar'}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 0)


    class TestEnvVar:
        def setup_method(self, method):
            os.environ["_TEST_FOO_"] = "hello"

        def teardown_method(self, method):
            del os.environ["_TEST_FOO_"]

        @pytest.mark.asyncio
        @pytest.mark.it('should allow accessing env vars')
        async def test_it_should_send_to_multiple_outputs(self):
            node = {
                "type": "function",
                "func": "msg.payload = env.get('_TEST_FOO_'); return msg;",
                "wires": [["3"]]
            }
            msgs = await run_with_single_node_ntimes(payload_type='str', payload='foo', node_json=node, nexpected=1, once=True, topic='bar')
            assert msgs[0]['topic'] == 'bar'
            assert msgs[0]['payload'] == 'hello'
