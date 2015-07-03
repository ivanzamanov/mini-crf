var path = require('path'),
    fs = require('fs'),
    async = require('async'),
    minimist = require('minimist'),
    _ = require('lodash'),
    search = require('./search'),
    printf = require('sprintf-js').sprintf;

var macConfig = {
    parallelProcesses: 1,
    parallelComparisons: 12,
    tempDir: '/Users/ivanzamanov/tmp/',
    synthDB: '/Users/ivanzamanov/SpeechSynthesis/db-synth-06-22-22-24.bin',
    testDB: '/Users/ivanzamanov/SpeechSynthesis/db-test-06-22-22-24.bin',
    trainingCommand: '/Users/ivanzamanov/SpeechSynthesis/mini-crf/src/main-opt',
    praatCommand: 'praat',
    synthScript: '/Users/ivanzamanov/SpeechSynthesis/mini-crf/scripts/concat.praat',
    compareScript: '/Users/ivanzamanov/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat',
    valueCachePath: '/Users/ivanzamanov/SpeechSynthesis/values-cache.json'
};

var linuxConfig = {
    parallelProcesses: 1,
    parallelComparisons: 12,
    tempDir: '/tmp/',
    synthDB: '/home/ivo/SpeechSynthesis/db-synth-06-14-15-11.bin',
    testDB: '/home/ivo/SpeechSynthesis/db-test-06-14-15-11.bin',
    trainingCommand: '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt',
    praatCommand: 'praat',
    synthScript: '/home/ivo/SpeechSynthesis/mini-crf/scripts/concat.praat',
    compareScript: '/home/ivo/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat',
    valueCachePath: '/home/ivo/SpeechSynthesis/values-cache.json'
};

var winConfig = {
    parallelProcesses: 1,
    parallelComparisons: 8,
    tempDir: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\temp',
    synthDB: '/home/ivo/SpeechSynthesis/db-synth.bin',
    testDB: '/home/ivo/SpeechSynthesis/db-test.bin',
    trainingCommand: '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt',
    praatCommand: '/usr/local/bin/praat-js',
    synthScript: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\mini-crf\\scripts\\concat.praat',
    compareScript: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\mini-crf\\scripts\\cepstral-distance.praat',
    valueCachePath: 'D:\\cygwin\\home\\ivo\\SpeechSynthesis\\values-cache.json'
};

var isWindows = require('os').platform() == 'win32';
var config = macConfig;
config.trainingArguments = ['--mode', 'train', '--synth-database', config.synthDB, '--test-database', config.testDB ];

function parseArguments() {
    var opts = {
        string: ""
    };
    var args = minimist(process.argv.slice(2), opts);
}

function isEmpty(line) {
    return !line;
}

var tempFileIndex = 0;
function getTempFile(ext) {
    if(!ext)
        ext = 'txt';
    return path.normalize(path.join(config.tempDir, 'tempFile' + ++tempFileIndex + '.' + ext));
}

function runCommand(command, input, callback) {
    var output = "";

    callback = _.once(callback);
    var child;
    if(isWindows) {
        command = ['D:\\cygwin\\bin\\bash.exe', 'D:\\cygwin\\home\\ivo\\bin\\stdin-wrap.sh', config.tempDir, command.join(' ')];
        child = require('child_process').spawn(command[0], command.slice(1), { cwd: 'D:\\cygwin\\bin\\' });
    } else {
        child = require('child_process').spawn(command[0], command.slice(1));
    }
    console.log(command.join(' '));
    /*var logFile = getTempFile('log');
     console.log('Log ' + command[0] + ' in ' + logFile);
     child.stderr.pipe(fs.createWriteStream(logFile));*/

    child.stdout.on('data', function (data) {
        output += data.toString('UTF-8');
    });

    child.on('error', function(err) {
        console.log('Child bad exit: ');
        console.log(err);
        process.exit(1);
    });
    child.on('exit', function(code) {
        if(code != 0) {
            console.log('Child bad exit:', code);
            process.exit(code);
        }
    });

    child.stdout.on('end', function() {
        callback(null, output);
    });

    child.stdin.end(input);
    return child;
}

function getTrainingCommand() {
    return [config.trainingCommand].concat(config.trainingArguments);
}

function getSynthCommand(synthInputPath, synthOutputPath) {
    return [config.praatCommand, config.synthScript, synthInputPath, synthOutputPath];
}

function getComparisonCommand(sound1, sound2) {
    return [config.praatCommand, config.compareScript, sound1, sound2];
}

function runTraining(runConfig, callback) {
    var coefs = runConfig.coefs,
        input = "",
        output = "";

    _.forEach(coefs, function(coef) { input += (coef.name + "=" + coef.value + "\n"); });

    runCommand(getTrainingCommand(), input, function(error, result) {
        if(error) {
            callback(error);
            return;
        }
        runConfig.trainingOutput = result;
        console.log('Training finished');
        callback(null, runConfig);
    });
}

function synthesize(synthInputPath, callback) {
    var synthesizedSound = getTempFile('wav');
    var synthCommand = getSynthCommand(synthInputPath, synthesizedSound);
    runCommand(synthCommand, '', function(err, output) {
        callback(null, synthesizedSound);
    });
}

function runComparison(originalSound, synthesizedSound, callback) {
    if(isWindows) {
        originalSound = originalSound.replace('/', '\\');
        originalSound = 'D:\\cygwin' + originalSound;
    }
    runCommand(getComparisonCommand(originalSound.trim(), synthesizedSound), '', callback);
}

function runSynthesis(runConfig, runSynthesisCallback) {
    var comparisonConfigs = [];
    var synthOutputs = runConfig.trainingOutput.split('----------');
    _.remove(synthOutputs, isEmpty);
    async.eachLimit(synthOutputs, config.parallelComparisons, function (synthOutput, synthCallback) {
        var lines = synthOutput.split('\n');
        _.remove(lines, isEmpty);
        var originalSound = lines[0];

        lines.splice(0, 1);
        var tempFile = getTempFile('synthin');
        fs.writeFileSync(tempFile, lines.join('\n'));

        async.waterfall([
            synthesize.bind(null, tempFile),
            runComparison.bind(null, originalSound),
            function(comparisonResult, callback) {
                console.log(originalSound + ' diff: ' + comparisonResult.trim());
                runConfig.result += parseFloat(comparisonResult.trim());
                if(isNaN(runConfig.result))
                    runConfig.result = Number.POSITIVE_INFINITY;
                callback();
            }],
                        synthCallback);
    }, runSynthesisCallback);
}

var allRunConfigs = [];
function trainWithCoefficients(coefs, callback) {
    var runConfig = {
        result: 0,
        coefs: coefs
    };
    allRunConfigs.push(runConfig);
    console.log('Training started: ');
    console.log(coefs);

    async.waterfall([
        runTraining.bind(null, runConfig),
        runSynthesis
    ], function() {
        console.log('Finished comparison for: ');
        delete runConfig.trainingOutput;
        console.log(runConfig);
        callback(runConfig.result);
    });
}

function mkRange(featureName, start, end, count) {
    if(!count) count = end - start;

    var result = {
        name: featureName,
        values: []
    }, step = (end - start) / count, i;

    for(i = 0; i < count; i++)
        result.values.push(start + step * i);

    return result;
}

var ranges = [
    mkRange('trans-mfcc', 1, 2),
    mkRange('trans-pitch', 999, 1000),
    mkRange('state-pitch', 999, 1000),
    mkRange('state-duration', 999, 1000)
];

function generateCoefficients(ranges, index, coefs, callback) {
    if (index == ranges.length) {
        callback(coefs);
        return;
    }

    var range = ranges[index];
    _.forEach(range.values, function(value) {
        coefs[range.name] = value;
        generateCoefficients(ranges, index + 1, coefs, callback);
    });
}

function trainAll() {
    var allCoefficients = [];
    generateCoefficients(ranges, 0, {}, function(coef) { allCoefficients.push(coef); });
    console.log('Combinations: ' + allCoefficients.length);
    async.eachLimit(allCoefficients, config.parallelProcesses, trainWithCoefficients, onAllFinished);
}

function onAllFinished() {
    console.log('All done');
    var min;
    _.forEach(allRunConfigs, function (config) {
        if(!min || config.result < min.result)
            min = config;
    });
    console.log('Best:');
    delete min.trainingOutput;
    console.log(min);
}

function goldenRange(from, to) {
    return [from, from + search.R * (to - from), to];
}

function collectArgs(ranges) {
    var args = [];
    _.forEach(ranges, function(range) {
        // The mid
        args.push({
            name: range.name,
            value: range.values[0]
        });
    });
    return args;
}

function logConverged(ranges) {
    console.log('Optimum at ' + search.dimensionsToString(ranges));
    _.forEach(ranges, function(range) {
        console.log(printf('%s=%f', range.name, range.values[0]));
    });
}

function trainGoldenSearch() {
    var ranges = [
        { name: 'trans-mfcc', values: goldenRange(-1000, 1000) },
        { name: 'trans-pitch', values: goldenRange(-1000, 1000) },
        { name: 'trans-ctx', values: goldenRange(-1000, 1000) },
        { name: 'state-pitch', values: goldenRange(-1000, 1000) },
        { name: 'state-duration', values: goldenRange(-1000, 1000) }
    ];
    var dimension = 0;
    var iterations = 1000;
    var multiParamFunc = search.MultiParamFunction(trainWithCoefficients, config.valueCachePath);

    var func = function(x, callback) {
        var args = collectArgs(ranges);
        args[dimension].value = x;
        multiParamFunc(args, callback);
    };

    function stepCallback(a, b, c) {
        // Order is a < b < c
        iterations--;
        console.log('Iteration: ' + (1000 - iterations));
        if(iterations === 0 || isConverged(ranges)) {
            logConverged(ranges);
        } else {
            ranges[dimension].values = [a, b, c];
            dimension = (dimension + 1) % ranges.length;

            var values = ranges[dimension].values;
            console.log('Searching in ' + search.dimensionsToString(ranges) + ' trying ' + dimension);
            search.goldenSearchStep(values[0], values[1], values[2], func, stepCallback);
        }
    }

    var values = ranges[dimension].values;
    console.log('Searching in ' + search.dimensionsToString(ranges));
    search.goldenSearchStep(values[0], values[1], values[2], func, stepCallback);
}

function isConverged(ranges) {
  var result = true;
  _.forEach(ranges, function(range) {
    result = result && (range.values[2] - range.values[0]) <= 1;
  });
  return result;
}

var bruteForce = false;
if(bruteForce) {
    trainAll();
} else {
    trainGoldenSearch();
}
