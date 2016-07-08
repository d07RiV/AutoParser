# AutoParser
Automatic datamining tool for D3
Branched from [SNOParser](https://github.com/d07RiV/SNOParser).

This tool can be build for Windows (Visual Studio 2013 or higher) or Linux (GCC 4.7 or higher). There is no flexible config, so if you want to adjust it for your own use, you will have to edit the source code.

Usage: `./autoparse <build-id>`

Where `<build-id>` is a 5 digit number of the build you want to get information from. The tool downloads all necessary files (around 100MB for the first version loaded, and a ~40MB for subsequent versions; it uses up 1-2GB disk space though) and creates .html files for the given version and diffs for the previous and current live versions. The whole process takes well under a minute to complete, making it suitable for automated use.

Required libraries:

* `zlib` - both Windows and Linux. A VS2013 library is attached.
* `curl` - Linux only. SSL is not required.

The following sections describe the default operation mode and what parts of code needs to be changed in order to adjust it.

### Work folder

The `Work` folder is always located alongside the executable and contains files necessary for the program to work. During program execution, a `cdncache` subfolder is created to store files loaded from the CDN, and `.js` files are created containing item and skill data for the given version.

### Output folder

The output folder is where the resulting `.html` files will be stored. It can be configured at the top of `autoparse.cpp` (defaulting to `/home/d3planner/public_html` on Linux). The output files are saved in three different subfolders: `game` will contain files named `(items|itemsets).<build-id>.html`; `diff` will contain files named `(items|itemsets|powers).<build-id>.html`; `powers` will contain files named `jspowers.<build-id>.js`.

The `.html` files will contain a header and a footer copied from `Work/templates`; between them there will be a line containing `{NAVBAR}` (which is intended to be replaced by the script that serves the pages) and item/skill/etc definitions. The script in the default headers requires jQuery located at `/external/jquery.min.js`, and requires item icons served at `/webgl/icons/<icon-id>`.

### Item data and formatting

If you need custom formatting for items and skills, there are two places that can be edited.

First, `parseItem`, `parseSetBonus` and `parsePower` functions in `miner.cpp` are responsible for creating JSON data that describes the items/skills. If you need to add additional fields, this is the place to do so.

Second, `makehtml` and `diff` functions generate the HTML code for given JSON data (or a pair of JSON's). These functions are fairly generic and should automatically support additional fields. If you want custom formatting, it would be easier to replace these with specialized functions instead.

### Extracting icons

This is not done by default, as it requires considerably more download/more diskspace for cache, and icons rarely change between versions so it is better suited for a separate manually-run tool (see SNOParser). However, this functionality is implemented in the `OpExtractIcons` function in `autoparser.cpp`, which can easily be added to `do_main`. The output is an `icons.wgz` archive located in the `Work` folder. The archive format is very simple: a 32-bit (little endian) integer indicating the number of files in the archive; then 12-byte blocks for every file, sorted by icon ID, containing `icon-id`, `file-offset` and `file-size` as 32-bit integers. The icon data is a plain PNG file.

### Logging

The tool can provide visual feedback (in the console) during execution. However, this is not suitable if you want to run it as an automated task, so you need to disable it by uncommenting the `#define NOLOGGER` line near the top of `logger.cpp` (if it is commented in the current commit).
