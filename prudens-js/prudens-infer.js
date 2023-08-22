const prudens = require('./prudens');
const parsers = require('./parsers');
const fs = require('fs');

let filepath = ".temp";
let constraints = "";

switch (process.argv.length) {
    case 4:
        constraints = process.argv[3];
    case 3:
        filepath = process.argv[2];
        break;
    default:
        break;
}

try {
    let data = fs.readFileSync(filepath, 'utf-8');
    const passed_data = data.toString().trim().split("\n");
    let kB = JSON.parse(passed_data[0]);
    try {
        let constraints_data = fs.readFileSync(constraints, 'utf-8').toString().trim();
        kB["constraints"] = parsers.parseConstraints(constraints_data);
    } catch (err) {
        kB["constraints"] = new Map();
    }

    for (let i = 1; i < passed_data.length; ++i) {
        const result = prudens.forwardChaining(kB, JSON.parse(passed_data[i])["context"]);

        const inferred = new Set();

        for (const key in result.graph) {
            result.graph[key] = result.graph[key].filter(
                (element) => !element.name.startsWith('$'));
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

        try {
            fs.appendFileSync(`${filepath}_1`, `${Array.from(inferred.values()).join(" ")}\n`);
        } catch (err) {
            return -1;
        }

    }
    try {
        fs.renameSync(`${filepath}_1`, filepath);
    } catch (err) {
        return -1;
    }
} catch (err) {
    console.log(`File '${filepath}' not found.`);
    return -1;
}
