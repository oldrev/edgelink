
var __utilModule = (function () {

    function __el_jsonDeepClone(v) {
        return JSON.parse(JSON.stringify(v));
    }

    function __el_cloneMsg(v) {
        var newMsg = jsonDeepClone(v);
        newMsg.id = evalEnv.generateMsgID();
        return newMsg;
    }

    // private:
    var privateVar = "I am private";
    function privateFunction() {
        console.log("I am a private function");
    }

    // public:
    return {

        /**
         * Safely clones a message object. This handles msg.req/msg.res objects that must
         * not be cloned.
         *
         * @param  {any}    msg - the message object to clone
         * @return {Object} the cloned message
         * @memberof @node-red/util_util
         */
        cloneMessage: function (msg) {
            if (typeof msg !== "undefined" && msg !== null) {
                // Temporary fix for #97
                // TODO: remove this http-node-specific fix somehow
                var req = msg.req;
                var res = msg.res;
                delete msg.req;
                delete msg.res;
                var m = __el_cloneMsg(msg);
                if (req) {
                    m.req = req;
                    msg.req = req;
                }
                if (res) {
                    m.res = res;
                    msg.res = res;
                }
                return m;
            }
            return msg;
        },
    };

})();


var RED = {
    util: __utilModule,

    env: {
        get: function(envVar) {
            return "dummy";
        }
    },
};







