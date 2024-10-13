import json
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
                {"t": rule, "v": rule_with}
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

async def _custom_flow_switch_test(flow, should_receive, send_payload):
    await _custom_flow_message_switch_test(flow, should_receive, {"payload": send_payload})

async def _singular_switch_test(rule, acheckall, should_receive, send_payload):
    flow = [
        {"id": "100", "type": "tab"},
        {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload", "rules": [{"t": rule}],
         "checkall": acheckall, "outputs": 1, "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "test-once"}
    ]
    await _custom_flow_switch_test(flow, should_receive, send_payload)

async def _two_field_switch_test(rule, rule_with, rule_with2, acheckall, should_receive, send_payload):
    flow = [
        {"id": "100", "type": "tab"},
        {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload", "rules":
         [{"t": rule, "v": rule_with, "v2": rule_with2}], "checkall": acheckall, "outputs": 1, "wires": [["2"]]},
        {"id": "2", "z": "100", "type": "test-once"}
    ]
    await _custom_flow_switch_test(flow, should_receive, send_payload)


@pytest.mark.describe('switch Node')
class TestSwitchNode:

    @pytest.mark.asyncio
    @pytest.mark.it('should check if payload equals given value')
    async def test_it_should_check_if_payload_equals_given_value(self):
        await _generic_switch_test("eq", "Hello", True, True, "Hello")

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload doesn't equal to desired string")
    async def test_it_should_return_nothing_when_the_payload_doesnt_equal_to_desired_string(self):
        await _generic_switch_test("eq", "Hello", True, False, "Hello!")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload NOT equals given value")
    async def test_it_should_check_if_payload_not_equals_given_value(self):
        await _generic_switch_test("neq", "Hello", True, True, "HELLO")

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload does equal to desired string")
    async def test_it_should_return_nothing_when_the_payload_does_equal_to_desired_string(self):
        await _generic_switch_test("neq", "Hello", True, False, "Hello")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload equals given numeric value")
    async def test_it_should_check_if_payload_equals_given_numeric_value(self):
        await _generic_switch_test("eq", 3, True, True, 3)

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload doesn\'t equal to desired numeric value")
    async def test_it_should_return_nothing_when_the_payload_doesnt_equal_to_desired_numeric_value(self):
        await _generic_switch_test("eq", 2, True, False, 4)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload NOT equals given numeric value")
    async def test_it_should_check_if_payload_not_equals_given_numeric_value(self):
        await _generic_switch_test("neq", 55667744, True, True, -1234)

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload does equal to desired numeric value")
    async def test_it_should_return_nothing_when_the_payload_does_equal_to_desired_numeric_value(self):
        await _generic_switch_test("neq", 10, True, False, 10)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is less than given value")
    async def test_it_should_check_if_payload_is_less_than_given_value(self):
        await _generic_switch_test("lt", 3, True, True, 2)

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload is not less than desired string")
    async def test_it_should_return_nothing_when_the_payload_is_not_less_than_desired_string(self):
        await _generic_switch_test("lt", 3, True, False, 4)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload less than equals given value")
    async def test_it_should_check_if_payload_less_than_equals_given_value(self):
        await _generic_switch_test("lte", 3, True, True, 3)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is greater than given value")
    async def test_it_should_check_if_payload_is_greater_than_given_value(self):
        await _generic_switch_test("gt", 3, True, True, 6)

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload is not greater than desired string")
    async def test_it_should_return_nothing_when_the_payload_is_not_greater_than_desired_string(self):
        await _generic_switch_test("gt", 3, True, False, -1)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is greater than/equals given value")
    async def test_it_should_check_if_payload_is_greater_than_equals_given_value(self):
        await _generic_switch_test("gte", 3, True, True, 3)

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload is not greater than desired string")
    async def test_it_should_return_nothing_when_the_payload_is_not_greater_than_desired_string_2(self):
        await _generic_switch_test("gt", 3, True, False, -1)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is greater than/equals given value")
    async def test_it_should_check_if_payload_is_greater_than_equals_given_value_2(self):
        await _generic_switch_test("gte", 3, True, True, 3)

    @pytest.mark.asyncio
    @pytest.mark.it("should match if a payload has a required property")
    async def test_it_should_match_if_a_payload_has_a_required_property(self):
        await _generic_switch_test("hask", "a", True, True, {"a": 1})

    @pytest.mark.asyncio
    @pytest.mark.it("should not match if a payload does not have a required property")
    async def test_it_should_not_match_if_a_payload_does_not_have_a_required_property(self):
        await _generic_switch_test("hask", "a", True, False, {"b": 1})

    @pytest.mark.asyncio
    @pytest.mark.it("should not match if the key is not a string")
    async def test_it_should_not_match_if_the_key_is_not_a_string(self):
        await _generic_switch_test("hask", 1, True, False, {"a": 1})

    @pytest.mark.asyncio
    @pytest.mark.it("should not match if the parent object does not exist - null")
    async def test_it_should_not_match_if_the_parent_object_does_not_exist_null(self):
        await _generic_switch_test("hask", "a", True, False, None)

    @pytest.mark.asyncio
    @pytest.mark.it("should not match if the parent object does not exist - undefined")
    async def test_it_should_not_match_if_the_parent_object_does_not_exist_undefined(self):
        await _generic_switch_test("hask", "a", True, False, None)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is between given values")
    async def test_it_should_check_if_payload_is_between_given_values(self):
        await _two_field_switch_test("btwn", "3", "5", True, True, 4)

    @pytest.mark.asyncio
    @pytest.mark.it('should check if payload is between given values in "wrong" order')
    async def test_it_should_check_if_payload_is_between_given_values_in_wrong_order(self):
        await _two_field_switch_test("btwn", "5", "3", True, True, 4)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is between given string values")
    async def test_it_should_check_if_payload_is_between_given_string_values(self):
        await _two_field_switch_test("btwn", "c", "e", True, True, "d")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not between given values")
    async def test_it_should_check_if_payload_is_not_between_given_values(self):
        await _two_field_switch_test("btwn", 3, 5, True, False, 12)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload contains given value")
    async def test_it_should_check_if_payload_contains_given_value(self):
        await _generic_switch_test("cont", "Hello", True, True, "Hello World!")

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload doesn't contain desired string")
    async def test_it_should_return_nothing_when_the_payload_doesnt_contain_desired_string(self):
        await _generic_switch_test("cont", "Hello", True, False, "This is not a greeting!")

    @pytest.mark.asyncio
    @pytest.mark.it("should match regex")
    async def test_it_should_match_regex(self):
        await _generic_switch_test("regex", "[abc]+", True, True, "abbabac")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type string")
    async def test_it_should_check_if_payload_if_of_type_string(self):
        await _generic_switch_test("istype", "string", True, True, "Hello")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type number")
    async def test_it_should_check_if_payload_if_of_type_number(self):
        await _generic_switch_test("istype", "number", True, True, 999)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type number 0")
    async def test_it_should_check_if_payload_if_of_type_number_0(self):
        await _generic_switch_test("istype", "number", True, True, 0)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type number NaN")
    async def test_it_should_check_if_payload_if_of_type_number_nan(self):
        await _generic_switch_test("istype", "number", True, False, float('nan'))

    # It doesn't work because we only got a JSON object not JS object
    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type number Infinity")
    async def test_it_should_check_if_payload_if_of_type_number_infinity(self):
        await _generic_switch_test("istype", "number", True, True, float('inf'))

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type boolean true")
    async def test_it_should_check_if_payload_if_of_type_boolean_true(self):
        await _generic_switch_test("istype", "boolean", True, True, True)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type boolean false")
    async def test_it_should_check_if_payload_if_of_type_boolean_false(self):
        await _generic_switch_test("istype", "boolean", True, True, False)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type array")
    async def test_it_should_check_if_payload_if_of_type_array(self):
        await _generic_switch_test("istype", "array", True, True, [1, 2, 3, "a", "b"])

    # It doesn't work because we only got a JSON object not JS object
    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type buffer")
    async def test_it_should_check_if_payload_if_of_type_buffer(self):
        await _generic_switch_test("istype", "buffer", True, True, bytearray(b"Hello"))

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type object")
    async def test_it_should_check_if_payload_if_of_type_object(self):
        await _generic_switch_test("istype", "object", True, True, {"a": 1, "b": "b", "c": True})

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type JSON string")
    async def test_it_should_check_if_payload_if_of_type_json_string(self):
        await _generic_switch_test("istype", "json", True, True, json.dumps({"a": 1, "b": "b", "c": True}))

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type JSON string (and fail if not)")
    async def test_it_should_check_if_payload_if_of_type_json_string_and_fail_if_not(self):
        await _generic_switch_test("istype", "json", True, False, "Hello")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type null")
    async def test_it_should_check_if_payload_if_of_type_null(self):
        await _generic_switch_test("istype", "null", True, True, None)

    # there is no `undefined` in Python neither Rust
    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload if of type undefined")
    async def test_it_should_check_if_payload_if_of_type_undefined(self):
        await _generic_switch_test("istype", "undefined", True, True, None)

    @pytest.mark.asyncio
    @pytest.mark.it('should handle flow context')
    async def test_it_should_handle_flow_context(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "change", "z": "100", "rules": [
                {"t": "set", "p": "foo", "pt": "flow", "to": "flowValue", "tot": "str"},
                {"t": "set", "p": "bar", "pt": "flow", "to": "flowValue", "tot": "str"},
            ], "reg": False, "name": "changeNode", "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "switch", "property": "foo", "propertyType": "flow",
             "rules": [{"t": "eq", "v": "bar", "vt": "flow"}],
                "checkall": "true", "outputs": 1, "wires": [["3"]]},
            {"id": "3", "z": "100", "type": "test-once"}
        ]
        injections = [
            {"nid": "1", "msg": {"payload": "value"}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["payload"] == "value"

    @pytest.mark.asyncio
    @pytest.mark.it('should handle persistable flow context')
    async def test_it_should_handle_persistable_flow_context(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "change", "z": "100", "rules": [
                {"t": "set", "p": "#:(memory1)::foo", "pt": "flow", "to": "flowValue", "tot": "str"},
                {"t": "set", "p": "#:(memory1)::bar", "pt": "flow", "to": "flowValue", "tot": "str"},
            ], "reg": False, "name": "changeNode", "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "switch", "property": "#:(memory1)::foo", "propertyType": "flow",
             "rules": [{"t": "eq", "v": "#:(memory1)::bar", "vt": "flow"}],
                "checkall": "true", "outputs": 1, "wires": [["3"]]},
            {"id": "3", "z": "100", "type": "test-once"}
        ]
        injections = [
            {"nid": "1", "msg": {"payload": "value"}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["payload"] == "value"

    @pytest.mark.asyncio
    @pytest.mark.it('should handle global context')
    async def test_it_should_handle_global_context(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "change", "z": "100", "rules": [
                {"t": "set", "p": "foo", "pt": "global", "to": "globalValue", "tot": "str"},
                {"t": "set", "p": "bar", "pt": "global", "to": "globalValue", "tot": "str"},
            ], "reg": False, "name": "changeNode", "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "switch", "property": "foo", "propertyType": "global",
             "rules": [{"t": "eq", "v": "bar", "vt": "global"}],
                "checkall": "true", "outputs": 1, "wires": [["3"]]},
            {"id": "3", "z": "100", "type": "test-once"}
        ]
        injections = [
            {"nid": "1", "msg": {"payload": "value"}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["payload"] == "value"

    @pytest.mark.asyncio
    @pytest.mark.it('should handle persistable global context')
    async def test_it_should_handle_persistable_global_context(self):
        flows = [
            {"id": "100", "type": "tab"},  # flow 1
            {"id": "1", "type": "change", "z": "100", "rules": [
                {"t": "set", "p": "#:(memory1)::foo", "pt": "global", "to": "globalValue", "tot": "str"},
                {"t": "set", "p": "#:(memory1)::bar", "pt": "global", "to": "globalValue", "tot": "str"},
            ], "reg": False, "name": "changeNode", "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "switch", "property": "#:(memory1)::foo", "propertyType": "global",
             "rules": [{"t": "eq", "v": "#:(memory1)::bar", "vt": "global"}],
                "checkall": "true", "outputs": 1, "wires": [["3"]]},
            {"id": "3", "z": "100", "type": "test-once"}
        ]
        injections = [
            {"nid": "1", "msg": {"payload": "value"}},
        ]
        msgs = await run_flow_with_msgs_ntimes(flows, injections, 1)
        assert msgs[0]["payload"] == "value"

    @pytest.mark.asyncio
    @pytest.mark.it('should use a nested message property to compare value - matches')
    async def test_it_should_use_a_nested_message_property_to_compare_value_matches(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload[msg.topic]", "rules": [
                {"t": "eq", "v": "bar"}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_message_switch_test(flow, True, {"topic": "foo", "payload": {"foo": "bar"}})

    @pytest.mark.asyncio
    @pytest.mark.it('should use a nested message property to compare value - no match')
    async def test_it_should_use_a_nested_message_property_to_compare_value_no_match(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload[msg.topic]", "rules": [
                {"t": "eq", "v": "bar"}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_message_switch_test(flow, False, {"topic": "foo", "payload": {"foo": "none"}})

    @pytest.mark.asyncio
    @pytest.mark.it('should use a nested message property to compare nested message property - matches')
    async def test_it_should_use_a_nested_message_property_to_compare_nested_message_property_matches(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload[msg.topic]", "rules": [
                {"t": "eq", "v": "payload[msg.topic2]", "vt": "msg"}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_message_switch_test(flow, True, {"topic": "foo", "topic2": "foo2", "payload": {"foo": "bar", "foo2": "bar"}})

    @pytest.mark.asyncio
    @pytest.mark.it('should use a nested message property to compare nested message property - no match')
    async def test_it_should_use_a_nested_message_property_to_compare_nested_message_property_no_match(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload[msg.topic]", "rules": [
                {"t": "eq", "v": "payload[msg.topic2]", "vt": "msg"}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_message_switch_test(flow, False, {"topic": "foo", "topic2": "foo2", "payload": {"foo": "bar", "foo2": "none"}})

    @pytest.mark.asyncio
    @pytest.mark.it('should match regex with ignore-case flag set true')
    async def test_it_should_match_regex_with_ignore_case_flag_set_true(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload", "rules": [
                {"t": "regex", "v": "onetwothree", "case": True}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_switch_test(flow, True, "oneTWOthree")

    @pytest.mark.asyncio
    @pytest.mark.it('should not match regex with ignore-case flag unset')
    async def test_it_should_not_match_regex_with_ignore_case_flag_unset(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload", "rules": [
                {"t": "regex", "v": "onetwothree"}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_switch_test(flow, False, "oneTWOthree")

    @pytest.mark.asyncio
    @pytest.mark.it('should not match regex with ignore-case flag set false')
    async def test_it_should_not_match_regex_with_ignore_case_flag_set_false(self):
        flow = [
            {"id": "100", "type": "tab"},
            {"id": "1", "z": "100", "type": "switch", "name": "switchNode", "property": "payload", "rules": [
                {"t": "regex", "v": "onetwothree", "case": False}], "checkall": True, "outputs": 1, "wires": [["2"]]},
            {"id": "2", "z": "100", "type": "test-once"}
        ]
        await _custom_flow_switch_test(flow, False, "oneTWOthree")

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload doesn't match regex")
    async def test_it_should_return_nothing_when_the_payload_doesnt_match_regex(self):
        await _generic_switch_test("regex", r"\d+", True, False, "This is not a digit")

    @pytest.mark.asyncio
    @pytest.mark.it("should return nothing when the payload doesn't contain desired string")
    async def test_it_should_return_nothing_when_the_payload_doesnt_contain_desired_string(self):
        await _generic_switch_test("cont", "Hello", True, False, "This is not a greeting!")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if input is true")
    async def test_it_should_check_if_input_is_true(self):
        await _singular_switch_test(True, True, True, True)

    @pytest.mark.asyncio
    @pytest.mark.it("sends nothing when input is false and checking for true")
    async def test_it_sends_nothing_when_input_is_false_and_checking_for_true(self):
        await _singular_switch_test(True, True, False, False)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if input is indeed false")
    async def test_it_should_check_if_input_is_indeed_false(self):
        await _singular_switch_test(False, True, True, False)

    @pytest.mark.asyncio
    @pytest.mark.it("sends nothing when input is false and checking for true")
    async def test_it_sends_nothing_when_input_is_false_and_checking_for_true_false_case(self):
        await _singular_switch_test(False, True, False, True)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (string)")
    async def test_it_should_check_if_payload_is_empty_string(self):
        await _singular_switch_test("empty", True, True, "")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (array)")
    async def test_it_should_check_if_payload_is_empty_array(self):
        await _singular_switch_test("empty", True, True, [])

    @pytest.mark.skip
    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (buffer)")
    async def test_it_should_check_if_payload_is_empty_buffer(self):
        await _singular_switch_test("empty", True, True, bytearray())

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (object)")
    async def test_it_should_check_if_payload_is_empty_object(self):
        await _singular_switch_test("empty", True, True, {})

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (non-empty string)")
    async def test_it_should_check_if_payload_is_empty_non_empty_string(self):
        await _singular_switch_test("empty", True, False, "1")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (non-empty array)")
    async def test_it_should_check_if_payload_is_empty_non_empty_array(self):
        await _singular_switch_test("empty", True, False, [1])

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (non-empty buffer)")
    async def test_it_should_check_if_payload_is_empty_non_empty_buffer(self):
        await _singular_switch_test("empty", True, False, bytearray(1))

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (non-empty object)")
    async def test_it_should_check_if_payload_is_empty_non_empty_object(self):
        await _singular_switch_test("empty", True, False, {"a": 1})

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (null)")
    async def test_it_should_check_if_payload_is_empty_null(self):
        await _singular_switch_test("empty", True, False, None)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (undefined)")
    async def test_it_should_check_if_payload_is_empty_undefined(self):
        await _singular_switch_test("empty", True, False, None)  # Assuming undefined maps to None

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is empty (0)")
    async def test_it_should_check_if_payload_is_empty_zero(self):
        await _singular_switch_test("empty", True, False, 0)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (string)")
    async def test_it_should_check_if_payload_is_not_empty_string(self):
        await _singular_switch_test("nempty", True, False, "")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (array)")
    async def test_it_should_check_if_payload_is_not_empty_array(self):
        await _singular_switch_test("nempty", True, False, [])

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (buffer)")
    async def test_it_should_check_if_payload_is_not_empty_buffer(self):
        await _singular_switch_test("nempty", True, False, [])

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (object)")
    async def test_it_should_check_if_payload_is_not_empty_object(self):
        await _singular_switch_test("nempty", True, False, {})

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (non-empty string)")
    async def test_it_should_check_if_payload_is_not_empty_non_empty_string(self):
        await _singular_switch_test("nempty", True, True, "1")

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (non-empty array)")
    async def test_it_should_check_if_payload_is_not_empty_non_empty_array(self):
        await _singular_switch_test("nempty", True, True, [1])

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (non-empty buffer)")
    async def test_it_should_check_if_payload_is_not_empty_non_empty_buffer(self):
        await _singular_switch_test("nempty", True, True, [1])

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (non-empty object)")
    async def test_it_should_check_if_payload_is_not_empty_non_empty_object(self):
        await _singular_switch_test("nempty", True, True, {"a": 1})

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (null)")
    async def test_it_should_check_if_payload_is_not_empty_null(self):
        await _singular_switch_test("nempty", True, False, None)

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (undefined)")
    async def test_it_should_check_if_payload_is_not_empty_undefined(self):
        await _singular_switch_test("nempty", True, False, None)  # Assuming undefined maps to None

    @pytest.mark.asyncio
    @pytest.mark.it("should check if payload is not empty (0)")
    async def test_it_should_check_if_payload_is_not_empty_zero(self):
        await _singular_switch_test("nempty", True, False, 0)
