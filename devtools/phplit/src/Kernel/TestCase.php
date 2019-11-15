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
// Created by polarboy on 2019/10/10.

namespace Lit\Kernel;

use Lit\Exception\ValueException;
use Lit\Utils\BooleanExpr;
use function Lit\Utils\escape_xml_attr;
use function Lit\Utils\quote_xml_attr;

/**
 * Test - Information on a single test instance.
 *
 * @package Lit\Kernel
 */
class TestCase
{
   /**
    * @var TestSuite $testSuite
    */
   private $testSuite;
   /**
    * @var string $pathInSuite
    */
   private $pathInSuite;

   /**
    * @var string $manualSpecifiedSourcePath
    */
   private $manualSpecifiedSourcePath;

   /**
    * @var TestingConfig $config
    */
   private $config;
   /**
    * @var string $filePath
    */
   private $filePath;
   /**
    * A list of conditions under which this test is expected to fail.
    * Each condition is a boolean expression of features and target
    * triple parts. These can optionally be provided by test format
    * handlers, and will be honored when the test result is supplied.
    *
    * @var array $xfails
    */
   private $xfails = array();
   /**
    * A list of conditions that must be satisfied before running the test.
    * Each condition is a boolean expression of features. All of them
    * must be True for the test to run.
    * FIXME should target triple parts count here too?
    *
    * @var array $requires
    */
   private $requires = array();
   /**
    * A list of conditions that prevent execution of the test.
    * Each condition is a boolean expression of features and target
    * triple parts. All of them must be False for the test to run.
    *
    * @var array $unsupported
    */
   private $unsupported = array();
   /**
    * @var TestResult $result
    */
   private $result = null;

   public function __construct(TestSuite $suite, array $pathInSuite, TestingConfig $config, string $filePath = null)
   {
      $this->testSuite = $suite;
      $this->pathInSuite = $pathInSuite;
      $this->config = $config;
      $this->filePath = $filePath;
   }

   public function setResult(TestResult $result)
   {
      if ($this->result != null) {
         throw new ValueException('test result already set');
      }
      $this->result = $result;
      // Apply the XFAIL handling to resolve the result exit code.
      try {
         if ($this->isExpectedToFail()) {
            if ($this->result->getCode() === TestResultCode::PASS()) {
               $this->result->setCode(TestResultCode::XPASS());
            } else if ($this->result->getCode() === TestResultCode::FAIL()) {
               $this->result->setCode(TestResultCode::XFAIL());
            }
         }
      } catch (ValueException $e) {
         // Syntax error in an XFAIL line.
         $this->result->setCode(TestResultCode::UNRESOLVED());
         $this->result->setOutput($e->getMessage());
      }
   }

   public function getFullName() : string
   {
      return $this->testSuite->getConfig()->getName() . ' :: ' . implode('/', $this->pathInSuite);
   }

   public function getFilePath() : string
   {
      if ($this->filePath) {
         return $this->filePath;
      }
      return $this->getSourcePath();
   }

   public function getSourcePath() : string
   {
      return $this->testSuite->getSourcePath($this->pathInSuite);
   }

   public function getExecPath() : string
   {
      return $this->testSuite->getExecPath($this->pathInSuite);
   }

   /**
    * Check whether this test is expected to fail in the current
    * configuration. This check relies on the test xfails property which by
    * some test formats may not be computed until the test has first been
    * executed.
    * Throws ValueError if an XFAIL line has a syntax error.
    *
    * @return bool
    */
   public function isExpectedToFail() : bool
   {
      $features = $this->config->getAvailableFeatures();
      $triple = $this->testSuite->getConfig()->getExtraConfig('target_triple', '');
      // Check if any of the xfails match an available feature or the target.
      foreach ($this->xfails as $item) {
         if ($item == '*') {
            return true;
         }
         // If this is a True expression of features and target triple parts,
         // it fails.
         try {
            if (BooleanExpr::evaluate($item, $features, $triple)) {
               return true;
            }
         } catch (ValueException $e) {
            throw new ValueException(sprintf("'Error in XFAIL list:\n%s'", $e->getMessage()));
         }
      }
      return false;
   }

   /**
    * A test is within the feature limits set by run_only_tests if
    * 1. the test's requirements ARE satisfied by the available features
    * 2. the test's requirements ARE NOT satisfied after the limiting
    * features are removed from the available features
    * Throws ValueError if a REQUIRES line has a syntax error.
    *
    * @return bool
    */
   public function isWithinFeatureLimits() : bool
   {
      if (empty($this->config->getLimitToFeatures())) {
         return true;
      }
      // Check the requirements as-is (#1)
      if ($this->getMissingRequiredFeatures()) {
         return false;
      }
      // Check the requirements after removing the limiting features (#2)
      $featuresMinusLimits = array();
      $availableFeatures = $this->config->getAvailableFeatures();
      $limitFeatures = $this->config->getLimitToFeatures();
      foreach ($availableFeatures as $feature) {
         if (!in_array($feature, $limitFeatures)) {
            $featuresMinusLimits[] = $feature;
         }
      }
      if (empty($this->getMissingRequiredFeaturesFromList($featuresMinusLimits))) {
         return false;
      }
      return true;
   }

   public function getMissingRequiredFeaturesFromList(array $features) : array
   {
      $evalResult = array();
      try {
         foreach ($this->requires as $item) {
            if (!BooleanExpr::evaluate($item, $features)) {
               $evalResult[] = $item;
            }
         }
         return $evalResult;
      } catch (ValueException $e) {
         throw new ValueException(sprintf("Error in REQUIRES list:\n%s", $e->getMessage()));
      }
   }

   /**
    * Returns a list of features from REQUIRES that are not satisfied."
    * Throws ValueError if a REQUIRES line has a syntax error.
    *
    * @return array
    */
   public function getMissingRequiredFeatures() : array
   {
      $features = $this->config->getAvailableFeatures();
      return $this->getMissingRequiredFeaturesFromList($features);
   }

   /**
    * Returns a list of features from UNSUPPORTED that are present
    * in the test configuration's features or target triple.
    * Throws ValueError if an UNSUPPORTED line has a syntax error.
    *
    * @return array
    */
   public function getUnsupportedFeatures(): array
   {
      $features = $this->config->getAvailableFeatures();
      $triple = $this->testSuite->getConfig()->getExtraConfig('target_triple', '');
      $evalResult = array();
      try {
         foreach ($this->unsupported as $item) {
            if (BooleanExpr::evaluate($item, $features, $triple)) {
               $evalResult[] = $item;
            }
         }
         return $evalResult;
      } catch (ValueException $e) {
         throw new ValueException(sprintf("Error in UNSUPPORTED list:\n%s", $e->getMessage()));
      }
   }

   /**
    *  Check whether this test should be executed early in a particular run.
    * This can be used for test suites with long running tests to maximize
    * parallelism or where it is desirable to surface their failures early.
    *
    * @return bool
    */
   public function isEarlyTest() : bool
   {
      return $this->testSuite->getConfig()->isEarly();
   }

   /**
    * Write the test's report xml representation to a file handle.
    *
    * @param $file
    */
   public function writeJUnitXML($file)
   {
      $testName = htmlspecialchars($this->pathInSuite[count($this->pathInSuite) - 1], ENT_XML1 | ENT_COMPAT, 'UTF-8');
      $testPath = array_slice($this->pathInSuite, 0, -1);
      $safeTestPath = array();
      foreach ($testPath as $path) {
         $safeTestPath[] = str_replace('.', '_', $path);
      }
      $safeName = str_replace('.', '_', $this->testSuite->getName());
      if ($safeTestPath) {
         $className = $safeName .'.' .implode('/', $safeTestPath);
      } else {
         $className = $safeName . '.' . $safeName;
      }
      $className = escape_xml_attr($className);
      $elapsedTime = $this->result->getElapsed();
      $testcaseTemplate = '<testcase classname=%s name=%s time=%s';
      $testcaseXml = sprintf($testcaseTemplate, quote_xml_attr($className),
         quote_xml_attr($testName), quote_xml_attr(sprintf("%.2f", $elapsedTime)));
      fwrite($file, $testcaseXml);
      if ($this->result->getCode()->isFailure()) {
         fwrite($file,">\n\t<failure ><![CDATA[");
         $output = $this->result->getOutput();
         // In the unlikely case that the output contains the CDATA terminator
         // we wrap it by creating a new CDATA block
         fwrite($file, str_replace("]]>","]]]]><![CDATA[>", $output));
         fwrite($file, "]]></failure>\n</testcase>");
      } else if ($this->result->getCode() === TestResultCode::UNSUPPORTED()) {
         $unsupportedFeatures = $this->getMissingRequiredFeatures();
         if (!empty($unsupportedFeatures)) {
            $skipMessage = "Skipping because of: " . join(',', $unsupportedFeatures);
         } else {
            $skipMessage = 'Skipping because of configuration.';
         }
         fwrite($file, sprintf(">\n\t<skipped message=%s />\n</testcase>\n", quote_xml_attr($skipMessage)));
      } else {
         fwrite($file, "/>");
      }
   }

   /**
    * @return TestSuite
    */
   public function getTestSuite(): TestSuite
   {
      return $this->testSuite;
   }

   /**
    * @param TestSuite $testSuite
    * @return TestCase
    */
   public function setTestSuite(TestSuite $testSuite): TestCase
   {
      $this->testSuite = $testSuite;
      return $this;
   }

   /**
    * @return string
    */
   public function getPathInSuite(): string
   {
      return $this->pathInSuite;
   }

   /**
    * @param string $pathInSuite
    * @return TestCase
    */
   public function setPathInSuite(string $pathInSuite): TestCase
   {
      $this->pathInSuite = $pathInSuite;
      return $this;
   }

   /**
    * @return TestingConfig
    */
   public function getConfig(): TestingConfig
   {
      return $this->config;
   }

   /**
    * @param TestingConfig $config
    * @return TestCase
    */
   public function setConfig(TestingConfig $config): TestCase
   {
      $this->config = $config;
      return $this;
   }

   /**
    * @param string $filePath
    * @return TestCase
    */
   public function setFilePath(string $filePath): TestCase
   {
      $this->filePath = $filePath;
      return $this;
   }

   /**
    * @return array
    */
   public function getXfails(): array
   {
      return $this->xfails;
   }

   /**
    * @param array $xfails
    * @return TestCase
    */
   public function setXfails(array $xfails): TestCase
   {
      $this->xfails = $xfails;
      return $this;
   }

   /**
    * @return array
    */
   public function getRequires(): array
   {
      return $this->requires;
   }

   /**
    * @param array $requires
    * @return TestCase
    */
   public function setRequires(array $requires): TestCase
   {
      $this->requires = $requires;
      return $this;
   }

   /**
    * @return array
    */
   public function getUnsupported(): array
   {
      return $this->unsupported;
   }

   /**
    * @param array $unsupported
    * @return TestCase
    */
   public function setUnsupported(array $unsupported): TestCase
   {
      $this->unsupported = $unsupported;
      return $this;
   }

   /**
    * @return TestResult
    */
   public function getResult(): TestResult
   {
      return $this->result;
   }

   /**
    * @return string
    */
   public function getManualSpecifiedSourcePath(): string
   {
      return $this->manualSpecifiedSourcePath;
   }

   /**
    * @param string $manualSpecifiedSourcePath
    * @return TestCase
    */
   public function setManualSpecifiedSourcePath(string $manualSpecifiedSourcePath): TestCase
   {
      $this->manualSpecifiedSourcePath = $manualSpecifiedSourcePath;
      return $this;
   }

   public function hasResult(): bool
   {
      return $this->result != null;
   }
}