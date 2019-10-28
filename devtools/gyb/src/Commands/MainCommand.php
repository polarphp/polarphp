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
// Created by polarboy on 2019/10/28.

namespace Gyb\Commands;

use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Output\OutputInterface;

class MainCommand extends Command
{
   protected static $defaultName = 'generate';

   protected function configure()
   {
      $this->setDescription('welcome use gyb syntax node generator');
      $this->setHelp($this->getHelpStr());
      $this->setupOptions();
      $this->setupArguments();
   }

   protected function setupOptions()
   {
      $this->addOption('param', 'D', InputOption::VALUE_REQUIRED|InputOption::VALUE_IS_ARRAY,
         "Add 'NAME' = 'VAL' Bindings to be set in the template's execution context", []);
      $this->addOption('output', 'o', InputOption::VALUE_OPTIONAL,
         'Output file (defaults to stdout)');
      $this->addOption('test', null, InputOption::VALUE_NONE,
         'Run a self-test');
      $this->addOption('verbose-test', null, InputOption::VALUE_NONE,
         'Run a verbose self-test');
      $this->addOption('dump', null, InputOption::VALUE_NONE,
         'Dump the parsed template to stdout');
      $this->addOption('line-directive', null, InputOption::VALUE_OPTIONAL, $this->getLineDirectiveDesc());

   }

   protected function setupArguments()
   {
      $this->addArgument("file", InputArgument::OPTIONAL,
         'Path to GYB template file (defaults to stdin)', null);
   }

   protected function execute(InputInterface $input, OutputInterface $output)
   {

   }

   protected function getHelpStr()
   {
      return <<<'EOF'
A GYB template consists of the following elements:

- Literal text which is inserted directly into the output
- Substitutions of the form ${php-expression}.  The PHP
  expression is converted to a string and the result is inserted
  into the output.
- PHP code delimited by <?php ... ?>.  Typically used to inject
  definitions (functions, classes, variable bindings) into the
  evaluation context of the template.  Common indentation is
  stripped, so you can add as much indentation to the beginning
  of this code as you like

Example template:

- Hello -

<?php
$x = 42
function succ($a) {
   return $a+1;
} 
?>
I can assure you that ${x} < ${succ(x)}
<?php 
if (intval($y) > 7) {
   for ($i in range(3)) { ?>
      echo "y is greater than seven!";
   <?php
   }
} else {
?>
   echo "y is less than or equal to seven";
<?php
}
?>

- The End. -

When run with "gyb -Dy=9", the output is
- Hello -

I can assure you that 42 < 43

y is greater than seven!
y is greater than seven!
y is greater than seven!

- The End. -
EOF;
   }

   private function getLineDirectiveDesc()
   {
      return <<<EOF
Line directive format string, which will be
provided 2 substitutions, `%%(line)d` and `%%(file)s`.
Example: `#sourceLocation(file: "%%(file)s", line: %%(line)d)`
The default works automatically with the `line-directive` tool,
which see for more information.
EOF;

   }
}