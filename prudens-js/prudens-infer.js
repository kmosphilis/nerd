const prudens = require('./prudens');
const parsers = require('./parsers');
const fs = require('fs');

let filepath = ".temp";
let constraints = "";
let save_inferring_rules = false;
let include_defeated = true;


switch (process.argv.length) {
    case 6:
        include_defeated = process.argv[5] != "d=0"
    case 5:
        if (process.argv[4].indexOf("s=") != -1) {
            save_inferring_rules = process.argv[4] == "s=1";
        } else {
            include_defeated = process.argv[4] != "d=0";
        }
    case 4:
        if (process.argv[3].indexOf("s=") != -1) {
            save_inferring_rules = process.argv[3] == "s=1";
        } else if (process.argv[3].indexOf("d=") != -1) {
            include_defeated = process.argv[3] != "d=0";
        } else {
            constraints = process.argv[3];
        }
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

    const inferring_rules = Array();

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
        inferring_rules.push(`${i + 1}: ${parsers.graphToString(result.graph)}`);

        if (include_defeated) {
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
        }

        try {
            fs.appendFileSync(`${filepath}_1`, `${Array.from(inferred.values()).join(" ")}\n`);
        } catch (err) {
            return -1;
        }

    }

    if (save_inferring_rules) {
        fs.appendFileSync(`${filepath}_1`, `${inferring_rules.join("\n\n")}`);
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
