var path = require('path'),
    fs = require('fs'),
    async = require('async'),
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
  compareScript: '/Users/ivanzamanov/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat',
  valueCachePath: 'values-cache.json'
};

var linuxConfig = {
  parallelProcesses: 1,
  parallelComparisons: 12,
  tempDir: '/tmp/',
  synthDB: '/home/ivo/SpeechSynthesis/db-synth-09-01-19-31.bin',
  testDB: '/home/ivo/SpeechSynthesis/db-test-09-01-19-31.bin',
  trainingCommand: '/home/ivo/SpeechSynthesis/mini-crf/src/main-opt',
  praatCommand: 'praat',
  compareScript: '/home/ivo/SpeechSynthesis/mini-crf/scripts/cepstral-distance.praat',
  valueCachePath: 'values-cache.json'
};

var config = linuxConfig;
config.trainingArguments = ['--mode', 'train', '--synth-database', config.synthDB, '--test-database', config.testDB ];

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
  var child = require('child_process').spawn(command[0], command.slice(1));
  console.log(command.join(' '));

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

function runComparison(originalSound, synthesizedSound, callback) {
  runCommand(getComparisonCommand(originalSound.trim(), synthesizedSound), '', callback);
}

function runAllComparisons(runConfig, runAllComparisonsCallback) {
  var synthOutputs = runConfig.trainingOutput.split('\n');
  _.remove(synthOutputs, isEmpty);
  async.eachLimit(synthOutputs, config.parallelComparisons, function (synthOutput, synthCallback) {
    var lines = synthOutput.split(' ');
    var originalSound = lines[0];
    var resynthFile = lines[1];

    async.waterfall([
      runComparison.bind(null, originalSound, resynthFile),
      function(comparisonResult, callback) {
        console.log(originalSound + ' diff: ' + comparisonResult.trim());
        runConfig.result += parseFloat(comparisonResult.trim());
        if(isNaN(runConfig.result))
          runConfig.result = Number.POSITIVE_INFINITY;
        callback();
      }], synthCallback);
  }, runAllComparisonsCallback);
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
    runAllComparisons
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
    { name: 'trans-ctx', values: goldenRange(-1000, 1000) },
    { name: 'trans-pitch', values: goldenRange(-1000, 1000) },
    { name: 'state-pitch', values: goldenRange(-1000, 1000) },
    { name: 'trans-mfcc', values: goldenRange(-1000, 1000) },
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

function isBetter(newVal, oldVal) {
  return !oldVal || (newVal && newVal > oldVal);
}

function nextVal(range) {
  if(range.values.current < range.values.to)
    return range.values.current = range.values.current + 1;
  else
    return range.values.current = undefined;
}


function seqRange(from, to) {
  return { from: from,
           to: to,
           current: from };
}

function optimizeSequential(ranges, currentRange, multiParamFunc, optimizeCallback) {
  var args = collectArgsSeq(ranges);
  var best;
  var bestVal;

  // And begin...
  multiParamFunc(args, onFunctionComplete);
  function onFunctionComplete(err, value) {
    if(err) {
      optimizeCallback(err);
      return;
    }
debugger
    if(isBetter(value, bestVal)) {
      best = currentRange.values.current;
      bestVal = value;
    }

    if(nextVal(currentRange)) {
      args = collectArgsSeq(ranges);
      multiParamFunc(args, onFunctionComplete);
    } else
      optimizeCallback(null, best);
  }
}

function collectArgsSeq(ranges) {
  var args = [];
  _.forEach(ranges, function(range) {
    // The mid
    args.push({
      name: range.name,
      value: range.values.current
    });
  });
  return args;
}

function trainSequential() {
  var ranges = [
    { name: 'trans-ctx', values: seqRange(1, 3) },
    { name: 'trans-pitch', values: seqRange(1, 3) },
    { name: 'state-pitch', values: seqRange(1, 3) },
    { name: 'trans-mfcc', values: seqRange(1, 3) },
    { name: 'state-duration', values: seqRange(1, 3) }
  ];

  var multiParamFunc = search.MultiParamFunction(trainWithCoefficients, config.valueCachePath);
  async.eachLimit(ranges, 1, function(range, callback) {
    optimizeSequential(ranges, range, multiParamFunc, onOptimized);

    function onOptimized(err, bestVal) {
      if(err) {
        callback(err);
        return;
      }
      range.values.current = bestVal;
      console.log(range.name + " best at " + range.current);
      callback();
    }
  }, function(err) {
       if(err)
         console.log(err);
       console.log("done");
     });
}

trainSequential();
//trainGoldenSearch();