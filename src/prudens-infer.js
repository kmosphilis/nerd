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

        let rules = []
        let inferredContext = 
            parsers.contextToString(result.facts).replace(/;/g,"").replace("true", "").trim();
        for (let key in result.graph) {
            result.graph[key] = result.graph[key].filter((element) => !element.name.startsWith('$'));
            if (result.graph[key].length > 0) {
                rules.push([]);
                for (let rule of result.graph[key]) {
                    rule.name = rule.name.replace("Rule", "");
                    rules[rules.length - 1].push(rule.name);
                }
                literals.push(`${key}, ${rules[rules.length - 1].length}`);
            }
        }
        rules.forEach((value, index, array) => array[index] = value.join(" "));
        fs.writeFile(".temp", `${inferredContext}\n${literals.join(" ")}\n${rules.join("\n")}`, (err) => {
            if (err) throw err;
            return;
        });
    }
});