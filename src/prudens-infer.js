const prudens = require('../prudens-js/prudens');
const parsers = require('../prudens-js/parsers');
const fs = require('fs');

fs.readFile(".temp", (err, data) => {
    if (!err) {
        const passed_data = data.toString().split("\n");
        let kB = JSON.parse(passed_data[0]);
        kB["constraints"] = new Map();
        const result = prudens.forwardChaining(kB, JSON.parse(passed_data[1])["context"]);

        const inferred = new Set();

        for (const key in result.graph) {
            result.graph[key] = result.graph[key].filter((element) => !element.name.startsWith('$'));
            if (result.graph[key].length > 0) {
                inferred.add(`${key}`);
            }
        }

        for (const item of result.defeatedRules) {
            if (item["by"]["name"].startsWith('$')) {
                const literal_sign = item["defeated"]["head"]
                const literal = parsers.literalToString(item["defeated"]["head"]);
                let opposed_literal;
                if (literal.startsWith('-')) {
                    opposed_literal = literal.slice(1);
                } else {
                    opposed_literal = "-" + literal;
                }

                if (!inferred.has(opposed_literal)) {
                    inferred.add(parsers.literalToString(item["defeated"]["head"]));
                }
            }
        }

        fs.writeFile(".temp", Array.from(inferred.values()).join(" "), (err) => {
            if (err) throw err;
            return;
        });
    } else {
        console.log("File not found.");
    }
});
