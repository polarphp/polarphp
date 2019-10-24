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
// Created by polarboy on 2019/10/24.
namespace Lit\Utils;
use Lit\Exception\ValueException;

/**
 * A lexical analyzer class for simple shell-like syntaxes.
 *
 * @package Lit\Utils
 */
class SysShLexer
{
   private $inStream;
   private $inFile;
   private $posix;
   private $eof;
   private $commenters = '#';
   /**
    * @var string $wordChars
    */
   private $wordChars;
   private $whitespace = " \t\r\n";
   private $whitespaceSplit = false;
   private $quotes = '\'"';
   private $escape = '\\';
   private $escapedquotes = '"';
   private $state = ' ';
   private $pushBack = [];
   private $lineNo = 1;
   private $debug = 0;
   private $token = '';
   private $fileStack = [];
   private $source = null;
   private $punctuationChars = [];
   /**
    * a push back queue used by lookahead logic
    *
    * @var array $pushBackChars
    */
   private $pushBackChars = [];

   public function __construct($inStream = null, $inFile = null, bool $posix = false,
                               $punctuationChars = false)
   {
      if (is_string($inStream)) {
         $str = $inStream;
         $inStream = fopen('php://memory', 'rw');
         fwrite($inStream, $str);
         rewind($inStream);
      }
      if ($inStream !== null) {
         $this->inStream = $inStream;
         $this->inFile = $inFile;
      } else {
         $this->inStream = STDIN;
      }
      $this->posix = $posix;
      if ($this->posix) {
         $this->eof = null;
      } else {
         $this->eof = '';
      }
      $this->wordChars = 'abcdfeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_';
      if ($this->posix) {
         $this->wordChars = 'ßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ';
      }
      if (!$punctuationChars) {
         $punctuationChars = '';
      } elseif ($punctuationChars === true) {
         $punctuationChars = '();<>|&';
      }
      $this->punctuationChars = $punctuationChars;
      if ($punctuationChars) {
         // these chars added because allowed in file names, args, wildcards
         $this->wordchars .= '~-./*?=';
         // remove any punctuation chars from wordchars
         $this->wordchars = str_replace(str_split($punctuationChars), '', $this->wordchars);
      }
   }

   /**
    * Push a token onto the stack popped by the get_token method
    */
   public function pushToken($token)
   {
      if ($this->debug >= 1) {
         printf("shlex: pushing token %s", $token);
      }
      array_unshift($this->pushBack, $token);
   }

   /**
    * Push an input source onto the lexer's input source stack.
    */
   public function pushSource($newStream, $newFile = null)
   {
      if (is_string($newStream)) {
         $str = $newStream;
         $newStream = fopen('php://memory', 'rw');
         fwrite($newStream, $str);
         rewind($newStream);
      }
      array_unshift($this->fileStack, [$this->inFile, $this->inStream, $this->lineNo]);
      $this->inFile = $newFile;
      $this->inStream = $newStream;
      $this->lineNo = 1;
      if ($this->debug) {
         if ($newFile != null) {
            printf('shlex: pushing to file %s', $this->infile);
         } else {
            printf('shlex: pushing to stream %s', $this->inStream);
         }
      }
   }

   /**
    * Pop the input source stack.
    */
   public function popSource()
   {
      fclose($this->inStream);
      list($inFile, $inStream, $lineNo) = array_shift($this->fileStack);
      $this->inFile = $inFile;
      $this->inStream = $inStream;
      $this->lineNo = $lineNo;
      if ($this->debug) {
         printf("shlex: popping to %s, line %d", $this->inStream, $this->lineNo);
      }
      $this->state = ' ';
   }

   /**
    * Get a token from the input stream (or from stack if it's nonempty)
    */
   public function getToken()
   {
      if ($this->pushBack) {
         $token = array_shift($this->pushBack);
         if ($this->debug >= 1) {
            printf("shlex: popping token %s", $token);
         }
         return $token;
      }
      // No pushback.  Get a token.
      $raw = $this->readToken();
      // Handle inclusions
      if ($this->source != null) {
         while ($raw == $this->source) {
            $spec = $this->sourceHook($this->readToken());
            if ($spec) {
               list($newFile, $newStream) = $spec;
               $this->pushSource($newStream, $newFile);
            }
            $raw = $this->getToken();
         }
      }
      // Maybe we got EOF instead?
      while ($raw == $this->eof) {
         if (!$this->fileStack) {
            return $this->eof;
         } else {
            $this->popSource();
            $raw = $this->getToken();
         }
      }
      // Neither inclusion nor EOF
      if ($this->debug >= 1) {
         if ($raw != $this->eof) {
            printf("shlex: token= %s", $raw);
         } else {
            printf("shlex: token=EOF");
         }
      }
      return $raw;
   }

   public function readToken()
   {
      $quoted = false;
      $escapedstate = ' ';
      while (true) {
         if ($this->punctuationChars && $this->pushBackChars) {
            $nextChar = array_pop($this->pushBackChars);
         } else {
            $nextChar = fread($this->inStream, 1);
         }
         if ($nextChar == "\n") {
            $this->lineNo += 1;
         }
         if ($this->debug > 3) {
            printf("shlex: in state %s I see character: %s", $this->state, $nextChar);
         }
         if ($this->state == null) {
            $this->token = ''; // past end of file
            break;
         } elseif ($this->state == ' ') {
            if (!$nextChar) {
               $this->state = null; // end of file
               break;
            } elseif (has_substr($this->whitespace, $nextChar)) {
               if ($this->debug >= 2) {
                  printf("shlex: I see whitespace in whitespace state");
               }
               if ($this->token || ($this->posix && $quoted)) {
                  break; // emit current token
               } else {
                  continue;
               }
            } elseif (has_substr($this->commenters, $nextChar)) {
               stream_get_line($this->inStream, 1024);
               $this->lineNo += 1;
            } elseif ($this->posix && has_substr($this->escape, $nextChar)) {
               $escapedstate = 'a';
               $this->state = $nextChar;
            } elseif (has_substr($this->wordChars, $nextChar)) {
               $this->token = $nextChar;
               $this->state = 'a';
            } elseif (has_substr($this->punctuationChars, $nextChar)) {
               $this->token = $nextChar;
               $this->state = 'c';
            } elseif(has_substr($this->quotes, $nextChar)) {
               if (!$this->posix) {
                  $this->token = $nextChar;
               }
               $this->state = $nextChar;
            } elseif ($this->whitespaceSplit) {
               $this->token = $nextChar;
               $this->state = 'a';
            } else {
               $this->token = $nextChar;
               if ($this->token || ($this->posix && $quoted)) {
                  break; // emit current token
               } else {
                  continue;
               }
            }
         } elseif (has_substr($this->quotes, $this->state)) {
            $quoted = true;
            if (!$nextChar) {
               if ($this->debug >= 2) {
                  printf("shlex: I see EOF in quotes state");
               }
               // XXX what error should be raised here?
               throw new ValueException("No closing quotation");
            }
            if ($nextChar == $this->state) {
               if (!$this->posix) {
                  $this->token .= $nextChar;
                  $this->state = ' ';
                  break;
               } else {
                  $this->state = 'a';
               }
            } elseif ($this->posix && has_substr($this->escape, $nextChar) &&
               has_substr($this->escapedquotes, $this->state)) {
               $escapedstate = $this->state;
               $this->state = $nextChar;
            } else {
               $this->token .= $nextChar;
            }
         } elseif (has_substr($this->escape, $this->state)) {
            if (!$nextChar) {
               // end of file
               if ($this->debug >= 2) {
                  printf("shlex: I see EOF in escape state");
               }
               // XXX what error should be raised here?
               throw new ValueException("No escaped character");
            }
            // In posix shells, only the quote itself or the escape
            // character may be escaped within quotes.
            if (has_substr($this->quotes, $escapedstate) &&
               $nextChar != $this->state && $nextChar != $escapedstate) {
               $this->token .= $this->state;
            }
            $this->token .= $nextChar;
            $this->state = $escapedstate;
         } elseif (in_array($this->state, ['a', 'c'])) {
            if (!$nextChar) {
               $this->state = null;
               break;
            } elseif (has_substr($this->whitespace, $nextChar)) {
               if ($this->debug >= 2) {
                  printf("shlex: I see whitespace in word state");
               }
               $this->state = ' ';
               if ($this->token || ($this->posix && $quoted)) {
                  break;
               } else {
                  continue;
               }
            } elseif (has_substr($this->commenters, $nextChar)) {
               stream_get_line($this->inStream, 1024);
               $this->lineNo += 1;
               if ($this->posix) {
                  $this->state = ' ';
                  if ($this->token || ($this->posix && $quoted)) {
                     break; // emit current token
                  } else {
                     continue;
                  }
               }
            } elseif ($this->state == 'c') {
               if (has_substr($this->punctuationChars, $nextChar)) {
                  $this->token .= $nextChar;
               } else {
                  if (!has_substr($this->whitespace, $nextChar)) {
                     $this->pushBackChars[] = $nextChar;
                  }
                  $this->state = ' ';
                  break;
               }
            } elseif ($this->posix && has_substr($this->quotes, $nextChar)) {
               $this->state = $nextChar;
            } elseif ($this->posix && has_substr($this->escape, $nextChar)) {
               $escapedstate = 'a';
               $this->state = $nextChar;
            } elseif (has_substr($this->wordChars, $nextChar) || has_substr($this->quotes, $nextChar)
               || $this->whitespaceSplit) {
               $this->token .= $nextChar;
            } else {
               if ($this->punctuationChars) {
                  $this->pushBackChars[] = $nextChar;
               } else {
                  array_unshift($this->pushBack, $nextChar);
               }
               if ($this->debug >= 2) {
                  printf("shlex: I see punctuation in word state");
               }
               $this->state = ' ';
               if ($this->token || ($this->posix && $quoted)) {
                  break; // emit current token
               } else {
                  continue;
               }
            }
         }
      }
      $result = $this->token;
      $this->token = '';
      if ($this->posix && !$quoted && $quoted && $result == '') {
         $result = null;
      }
      if ($this->debug > 1) {
         if ($result) {
            printf("shlex: raw token= %s", $result);
         } else {
            printf("shlex: raw token=EOF");
         }
      }
      return $result;
   }

   public function toArray()
   {
      $tokens = [];
      $token = $this->getToken();
      while ($token != $this->eof) {
         $tokens[] = $token;
         $token = $this->getToken();
      }
      return $tokens;
   }

   /**
    * Hook called on a filename to be sourced.
    *
    * @param $newFile
    */
   public function sourceHook($newFile)
   {
      if ($newFile[0] == '"') {
         $newFile = substr($newFile, 1, -1);
      }
      // This implements cpp-like semantics for relative-path inclusion.
      if (is_string($this->inFile) && !is_absolute_path($newFile)) {
         $newFile = dirname($this->inFile) .DIRECTORY_SEPARATOR . $newFile;
      }
      return [$newFile, fopen($newFile, 'r')];
   }

   /**
    * Emit a C-compiler-like, Emacs-friendly error-message leader.
    *
    * @param null $infile
    * @param null $lineNo
    */
   public function errorLeader($infile = null, $lineNo = null)
   {
      if ($infile == null) {
         $infile = $this->inFile;
      }
      if ($lineNo == null) {
         $lineNo = $this->lineNo;
      }
      return sprintf("\"%s\", line %d: ", $infile, $lineNo);
   }

   /**
    * @return bool
    */
   public function isWhitespaceSplit(): bool
   {
      return $this->whitespaceSplit;
   }

   /**
    * @param bool $whitespaceSplit
    * @return SysShLexer
    */
   public function setWhitespaceSplit(bool $whitespaceSplit): SysShLexer
   {
      $this->whitespaceSplit = $whitespaceSplit;
      return $this;
   }

   /**
    * @return string
    */
   public function getCommenters(): string
   {
      return $this->commenters;
   }

   /**
    * @param string $commenters
    * @return SysShLexer
    */
   public function setCommenters(string $commenters): SysShLexer
   {
      $this->commenters = $commenters;
      return $this;
   }

}
