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
// Created by polarboy on 2019/10/08.

use Symfony\Component\Dotenv\Dotenv;

set_time_limit(0);
$script = $_SERVER['SCRIPT_NAME'];
// TODO work around for windows
if ($script[0] == '/') {
   define('LIT_ROOT_DIR', dirname(dirname($_SERVER['SCRIPT_NAME'])));
} else {
   define('LIT_ROOT_DIR', dirname(dirname(getcwd().'/'.$_SERVER['SCRIPT_NAME'])));
}

require LIT_ROOT_DIR . '/vendor/autoload.php';

// Load cached env vars if the .env.local.php file exists
// Run "composer dump-env prod" to create it (requires symfony/flex >=1.2)
if (is_array($env = @include LIT_ROOT_DIR . '/.env.local.php')) {
   foreach ($env as $k => $v) {
      $_ENV[$k] = $_ENV[$k] ?? (isset($_SERVER[$k]) && 0 !== strpos($k, 'HTTP_') ? $_SERVER[$k] : $v);
   }
} elseif (!class_exists(Dotenv::class)) {
   throw new RuntimeException('Please run "composer require symfony/dotenv" to load the ".env" files configuring the application.');
} else {
   // load all the .env files
   (new Dotenv(false))->loadEnv(LIT_ROOT_DIR . '/.env');
}

$_SERVER += $_ENV;
$_SERVER['APP_ENV'] = $_ENV['APP_ENV'] = ($_SERVER['APP_ENV'] ?? $_ENV['APP_ENV'] ?? null) ?: 'dev';
$_SERVER['APP_DEBUG'] = $_SERVER['APP_DEBUG'] ?? $_ENV['APP_DEBUG'] ?? 'prod' !== $_SERVER['APP_ENV'];
$_SERVER['APP_DEBUG'] = $_ENV['APP_DEBUG'] = (int)$_SERVER['APP_DEBUG'] || filter_var($_SERVER['APP_DEBUG'], FILTER_VALIDATE_BOOLEAN) ? '1' : '0';

$extraFiles = array(
   "src/Utils/UtilFuncs.php"
);
foreach ($extraFiles as $file) {
   include LIT_ROOT_DIR .DIRECTORY_SEPARATOR . $file;
}