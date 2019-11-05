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
// Created by polarboy on 2019/10/29.

use Gyb\Syntax\Trivia;

return array(
   new Trivia('Space', "A space ' ' character.", 0, [' ']),
   new Trivia('Tab', "A tab '\\t' character.", 1, ['\t']),
   new Trivia('VerticalTab', "A vertical tab '\\v' character.", 2, ['\v']),
   new Trivia('Formfeed', "A form-feed 'f' character.", 3, ['\f']),
   new Trivia('Newline', "A newline '\\n' character.", 4, ['\n'], [], true),
   new Trivia('CarriageReturn', "A newline '\\r' character.", 5, ['\r'],
      [], true),
   new Trivia('CarriageReturnLineFeed', "A newline consists of contiguous '\\r' and '\\n' characters.", 6,
      ['\r', '\n'], [], true),
   new Trivia('Backtick', "A backtick '`' character, used to escape identifiers.", 7, ['`']),
   new Trivia('LineComment', "A developer line comment, starting with '//'", 8, [],
      [],false, true),
   new Trivia('BlockComment', "A developer block comment, starting with '/*' and ending with", 9,
      [], [], false, true),
   new Trivia('DocLineComment', "A documentation line comment, starting with '///'.",
      10, [], [], false, true),
   new Trivia('DocBlockComment', "A documentation block comment, starting with '/**' and ending with '*/'.",
   11, [], [], false, true),
   new Trivia('GarbageText', "Any skipped garbage text.", 12)
);