const prudens = require('../prudens-js/prudens');
const parsers = require('../prudens-js/parsers');
const fs = require('fs');

fs.readFile(".temp", (err, data) => {
    if (!err) {
        const passed_data = data.toString().split("\n");
        let test = JSON.parse(passed_data[0]);
        test["constraints"] = new Map();
        const result = prudens.forwardChaining(test, JSON.parse(passed_data[1])["context"]);

        const graphKeys = Object.keys(result.graph);

        let literals = [];

        for (let key in result.graph) {
            result.graph[key] = result.graph[key].filter((element) => !element.name.startsWith('$'));
            if (result.graph[key].length > 0) {
                literals.push(`${key}`);
            }
        }
        fs.writeFile(".temp", literals.join(" "), (err) => {
            if (err) throw err;
            return;
        });
    }
});