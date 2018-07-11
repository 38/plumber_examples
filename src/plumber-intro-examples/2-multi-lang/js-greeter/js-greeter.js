using("pservlet");
pservlet.setupCallbacks({
    init: function() {
        return {
            name  :  pservlet.pipe.define("input", pservlet.pipe.flags.INPUT),
            output:  pservlet.pipe.define("output", pservlet.pipe.flags.OUTPUT)
        };
    },
    exec: function(context) {
        var name = pservlet.pipe.read(context.name, 1024).readString();
        pservlet.pipe.write(context.output, name + "hello form Javascript\n");
    }
});
