import fs from "node:fs";

const d = "JSONTestSuite/test_parsing/";
fs.readdirSync(d).forEach((n) => {
  console.log(n);
  fs.writeFileSync("js-results/" + n, (() => {
    try {
      return "ok  " + JSON.stringify(JSON.parse(fs.readFileSync(d + n).toString()));
    } catch (err) {
      return "err " + err.toString();
    }
  })());
})