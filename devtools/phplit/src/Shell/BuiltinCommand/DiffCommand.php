<?php
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/18.

namespace Lit\Shell\BuiltinCommand;


use Lit\Exception\InternalShellException;
use Lit\Shell\ShCommandInterface;
use Lit\Shell\ShellCommandResult;
use Lit\Utils\ShellEnvironment;
use Symfony\Component\Console\Input\ArgvInput;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use function Lit\Utils\expand_glob_expressions;
use function Lit\Utils\is_absolute_path;

class DiffCommand implements BuiltinCommandInterface
{
   /**
    * @var bool $stripTrailingCr
    */
   private $stripTrailingCr = false;
   /**
    * @var bool $ignoreAllSpace
    */
   private $ignoreAllSpace = false;
   /**
    * @var bool $ignoreSpaceChange
    */
   private $ignoreSpaceChange = false;

   /**
    * @var resource $stdout
    */
   private $stdout;

   /**
    * @var resource $stderr
    */
   private $stderr;

   public function __construct()
   {
      $definitions = new InputDefinition();
      $definitions->addOption(new InputOption("ignore-all-space", 'w', InputOption::VALUE_NONE,
         "ignore-all-space"));
      $definitions->addOption(new InputOption("ignore-space-change", 'b', InputOption::VALUE_NONE,
         "ignore-space-change"));
      $definitions->addOption(new InputOption("unified-diff", 'u', InputOption::VALUE_NONE,
         "unified-diff"));
      $definitions->addOption(new InputOption("recursive-diff", 'r', InputOption::VALUE_NONE,
         "recursive-diff"));
      $definitions->addOption(new InputOption("strip-trailing-cr", null, InputOption::VALUE_NONE,
         "strip-trailing-cr"));
      $definitions->addArgument(new InputArgument("items", InputArgument::IS_ARRAY, "directories to created"));
      $this->definitions = $definitions;
   }

   public function execute(ShCommandInterface $cmd, ShellEnvironment $shenv)
   {
      try {
         $args = expand_glob_expressions($cmd->getArgs(), $shenv->getCwd());
         $input = new ArgvInput($args, $this->definitions);
      } catch (\Exception $e) {
         throw new InternalShellException($cmd, sprintf("Unsupported: 'diff': %s", $e->getMessage()));
      }
      $this->ignoreAllSpace = $input->getOption('ignore-all-space');
      $this->ignoreSpaceChange = $input->getOption('ignore-space-change');
      $recursiveDiff = $input->getOption('recursive-diff');
      $this->stripTrailingCr = $input->getOption('strip-trailing-cr');
      $items = $input->getArgument('items');
      if (count($items) != 2) {
         throw new InternalShellException($cmd, "Error:  missing or extra operand");
      }
      $this->stdout = fopen('php://memory','rw');
      $this->stderr = fopen('php://memory','rw');
      $exitCode = 0;
      try {
         $filePaths = $dirTrees = [];
         foreach ($items as $file) {
            if (!is_absolute_path($file)) {
               $file = $shenv->getCwd().DIRECTORY_SEPARATOR.$file;
            }
            if ($recursiveDiff) {
               $dirTrees[] = $this->getDirTree($file);
            } else {
               $filePaths[] = $file;
            }
         }
         if (!$recursiveDiff) {
            $exitCode = $this->compareTwoFiles($filePaths);
         } else {
            $exitCode = $this->compareDirTrees($dirTrees);
         }
      } catch (\Exception $e) {
         $this->writeToStderr(sprintf("Error: 'diff' command failed, %s\n", $e->getMessage()));
         $exitCode = 1;
      }
      rewind($this->stdout);
      rewind($this->stderr);
      return new ShellCommandResult($cmd, stream_get_contents($this->stdout), stream_get_contents($this->stderr),
         $exitCode, false);
   }

   private function writeToStdout(string $text)
   {
      fwrite($this->stdout, $text);
   }

   private function writeToStderr(string $text)
   {
      fwrite($this->stderr, $text);
   }

   /**
    * Tree is a tuple of form (dirname, child_trees).
    * An empty dir has child_trees = [], a file has child_trees = None.
    * @param string $path
    * @param string $baseDir
    */
   private function getDirTree(string $path, string $baseDir = "")
   {
      $childTrees = [];
      $diter = new \DirectoryIterator($baseDir.DIRECTORY_SEPARATOR.$path);
      foreach ($diter as $entry) {
         if (!$entry->isDot() && $entry->isDir()) {
            $childTrees[] = $this->getDirTree($entry->getFilename(), $entry->getPath());
         }
         if ($entry->isFile()) {
            $childTrees[] = [$entry->getFilename(), null];
         }
      }
      sort($childTrees);
      return [$path, $childTrees];
   }

   private function compareTwoFiles(array $filePaths): int
   {
      $compareBytes = false;
      $fileLines = null;
      foreach ($filePaths as $file) {
         $fhandle = fopen($file, 'r');
         // detected encoding
         $bytes = fread($fhandle, 1024);
         if (false === mb_detect_encoding($bytes, "UTF-8")) {
            $compareBytes = true;
         }
      }
      if ($compareBytes) {
         return $this->compareTwoBinaryFiles($filePaths[0], $filePaths[1]);
      }
      return $this->compareTwoTextFiles($filePaths);
   }

   private function compareTwoBinaryFiles(string $leftFilePath, string $rightFilePath): int
   {
      $leftFileHandle = fopen($leftFilePath, 'rb');
      $rightFileHandle = fopen($rightFilePath, 'rb');
      $lhsSize = filesize($leftFilePath);
      $rhsSize = filesize($rightFilePath);
      if ($lhsSize != $rhsSize) {
         return 1;
      }
      while (!feof($leftFileHandle)) {
         $lhsHash = md5(fread($leftFileHandle, 1024));
         $rhsHash = md5(fread($rightFileHandle, 1024));
         if ($lhsHash != $rhsHash) {
            $this->writeToStdout(sprintf("*** %s\n", $leftFilePath));
            $this->writeToStdout(sprintf("--- %s\n", $rightFilePath));
            $this->writeToStdout("binary files is different\n");
            return 1;
         }
      }
      return 0;
   }

   private function compareTwoTextFiles(array $filePaths): int
   {
      $fileLines = [];
      foreach ($filePaths as $file) {
         $fileLines[] = file($file, FILE_IGNORE_NEW_LINES);
      }
      $exitCode = 0;
      $stripTrailingCr = $this->stripTrailingCr;
      $ignoreAllSpace = $this->ignoreAllSpace;
      $ignoreSpaceChange = $this->ignoreSpaceChange;
      foreach ($fileLines as $idx => $lines) {
         foreach ($lines as $lineNumber => $line) {
            $lines[$lineNumber] = $this->filterLine($line, $stripTrailingCr, $ignoreAllSpace, $ignoreSpaceChange);
         }
         $fileLines[$idx] = $lines;
      }
      $diff = new \Diff($fileLines[0], $fileLines[1]);
      $renderer = new \Diff_Renderer_Text_Unified();
      $ditems = $diff->render($renderer);
      if (!empty($ditems)) {
         $this->writeToStdout(sprintf("*** %s\n", $filePaths[0]));
         $this->writeToStdout(sprintf("--- %s\n", $filePaths[1]));
         $this->writeToStdout($ditems);
         $exitCode = 1;
      }
      return $exitCode;
   }

   private function renderDiffs(array $filePaths, array $fileLines, array $diffItems)
   {
      $rmLines = [];
      $addLines = [];
      foreach ($diffItems as $item) {
         if ($item instanceof DiffOpAdd) {
            $addLines[] = $item->getNewValue();
         } elseif ($item instanceof DiffOpRemove) {
            $rmLines[] = $item->getOldValue();
         }
      }
      $this->writeToStdout(sprintf("--- %s\n", $filePaths[0]));
      $this->writeToStdout(sprintf("+++ %s\n", $filePaths[1]));
      $leftLines = $fileLines[0];
      $rightLines = $fileLines[1];
      $leftLineCount = count($leftLines);
      $rightLineCount = count($rightLines);
      $addIndex = 0;
      $rmIndex = 0;
      for($i = 0, $j = 0; $i < $leftLineCount && $j < $rightLineCount; ++$i, ++$j) {
         $lhs = $leftLines[$i];
         $rhs = $rightLines[$j];
         if ($lhs == $rhs) {
            $this->writeToStdout($lhs);
         } else {
            $this->writeToStdout(sprintf("- %s", $rmLines[$rmIndex++]));
            $this->writeToStdout(sprintf("+ %s", $addLines[$addIndex++]));
         }
      }
   }

   private function filterLine(string $line, $stripTrailingCr = false, $ignoreAllSpace = false,
                       $ignoreSpaceChange = false)
   {
      if ($stripTrailingCr) {
         $line = trim($line, "\r");
      }
      if ($ignoreAllSpace) {
         $line = str_replace(' ', '', $line);
      } elseif ($ignoreSpaceChange) {
         $line = preg_replace('/\s+/', ' ', $line);
      }
      return $line;
   }

   /**
    * Dirnames of the trees are not checked, it's caller's responsibility,
    * as top-level dirnames are always different. Base paths are important
    * for doing os.walk, but we don't put it into tree's dirname in order
    * to speed up string comparison below and while sorting in getDirTree.
    *
    * @param array $dirs
    * @param array $basePaths
    */
   private function compareDirTrees(array $dirTrees, $basePaths = ['', ''])
   {
      $leftTree = $dirTrees[0];
      $rightTree = $dirTrees[1];
      $leftBase = $basePaths[0];
      $rightBase = $basePaths[1];
      // Compare two files or report file vs. directory mismatch.
      if ($leftTree[1] === null && $rightTree[1] === null) {
         return $this->compareTwoFiles([
            $leftBase.DIRECTORY_SEPARATOR.$leftTree[0],
            $rightBase.DIRECTORY_SEPARATOR.$rightTree[0]
         ]);
      }
      if ($leftTree[1] === null && $rightTree[1] !== null) {
         $this->printFileVsDir($leftBase.DIRECTORY_SEPARATOR.$leftTree[0],
            $rightBase.DIRECTORY_SEPARATOR.$rightTree[0]);
         return 1;
      }
      if ($leftTree[1] !== null && $rightTree[1] === null) {
         $this->printDirVsFile($leftBase.DIRECTORY_SEPARATOR.$leftTree[0],
            $rightBase.DIRECTORY_SEPARATOR.$rightTree[0]);
         return 1;
      }
      // Compare two directories via recursive use of compareDirTrees.
      $exitCode = 0;
      $leftNames = [];
      $rightNames = [];
      foreach ($leftTree[1] as $node) {
         $leftNames[] = $node[0];
      }
      foreach ($rightTree[1] as $node) {
         $rightNames[] = $node[0];
      }
      $l = $r = 0;
      while ($l < count($leftNames) && $r < count($rightNames)) {
         // Names are sorted in getDirTree, rely on that order.
         if ($leftNames[$l] < $rightNames[$r]) {
            $exitCode = 1;
            $this->printOnlyIn($leftBase, $leftTree[0], $leftNames[$l]);
            $l += 1;
         } elseif ($leftNames[$l] > $rightNames[$r]) {
            $exitCode = 1;
            $this->printOnlyIn($rightBase, $rightTree[0], $rightNames[$r]);
            $r += 1;
         } else {
            $exitCode |= $this->compareDirTrees(
               [$leftTree[1][$l], $rightTree[1][$r]],
               [$leftBase . DIRECTORY_SEPARATOR . $leftTree[0], $rightBase . DIRECTORY_SEPARATOR . $rightTree[0]]
            );
            $l += 1;
            $r += 1;
         }
      }
      // At least one of the trees has ended. Report names from the other tree.
      while ($l < count($leftNames)) {
         $exitCode = 1;
         $this->printOnlyIn($leftBase, $leftTree[0], $leftNames[$l]);
         $l += 1;
      }
      while ($r < count($rightNames)) {
         $exitCode = 1;
         $this->printOnlyIn($rightBase, $rightTree[0], $rightNames[$r]);
         $r += 1;
      }
      return $exitCode;
   }

   private function printOnlyIn($baseDir, $path, $name)
   {
      $this->writeToStdout(sprintf("Only in %s: %s\n",
         $baseDir.DIRECTORY_SEPARATOR.$path, $name));
   }

   private function printDirVsFile($dirPath, $filePath)
   {
      if (filesize($filePath)) {
         $msg = "File %s is a directory while file %s is a regular file";
      } else {
         $msg = "File %s is a directory while file %s is a regular empty file";
      }
      $this->writeToStdout(sprintf($msg, $dirPath, $filePath));
   }

   private function printFileVsDir($filePath, $dirPath)
   {
      if (filesize($filePath) > 0) {
         $msg = "File %s is a regular file while file %s is a directory";
      } else {
         $msg = "File %s is a regular empty file while file %s is a directory";
      }
      $this->writeToStdout(sprintf($msg, $filePath, $dirPath));
   }
}