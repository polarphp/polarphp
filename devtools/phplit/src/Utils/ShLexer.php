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
// Created by polarboy on 2019/10/12.
namespace Lit\Utils;
use Lit\Shell\GlobItemCommand;
use Lit\Exception\ValueException;

class ShLexer
{
   /**
    * @var string $data
    */
   private $data;
   /**
    * @var int $pos
    */
   private $pos;
   /**
    * @var int $end
    */
   private $end;
   /**
    * @var bool $win32Escapes
    */
   private $win32Escapes;

   public function __construct(string $data, bool $win32Escapes = false)
   {
      $this->data = $data;
      $this->win32Escapes = $win32Escapes;
      $this->pos = 0;
      $this->end = strlen($data);
   }

   public function lex() : iterable
   {
      while ($this->pos != $this->end) {
         if (ctype_space($this->look())) {
            $this->eat();
         } else {
            yield $this->lexOneToken();
         }
      }
   }

   private function eat()
   {
      $char = $this->data[$this->pos];
      ++$this->pos;
      return $char;
   }

   private function look()
   {
      return $this->data[$this->pos];
   }

   /**
    * maybe_eat(c) - Consume the character c if it is the next character,
    * returning True if a character was consumed.
    *
    * @param $char
    */
   private function maybeEat($char): bool
   {
      if ($this->data[$this->pos] == $char) {
         ++$this->pos;
         return true;
      }
      return false;
   }

   private function lexArgFast($char)
   {
      // Get the leading whitespace free section.
      $chunk = substr($this->data, $this->pos - 1);
      $chunk = preg_split('/[\s\t\n\r\0\x0B]+/', $chunk, 2)[0];
      // If it has special characters, the fast path failed.
      if (has_substr($chunk, '|')  || has_substr($chunk, '&') ||
         has_substr($chunk, '<')  || has_substr($chunk, '>') ||
         has_substr($chunk, "'")  || has_substr($chunk, '"') ||
         has_substr($chunk, '\\') || has_substr($chunk, ';')) {
         return null;
      }
      $this->pos = $this->pos - 1 + strlen($chunk);
      if (has_substr($chunk, '*') || has_substr($chunk, '?')) {
         return new GlobItemCommand($chunk);
      }
      return $chunk;
   }

   private function lexArgSlow($char)
   {
      if (has_substr('\'"', $char)) {
         $str = $this->lexArgQuoted($char);
      } else {
         $str = $char;
      }
      $unquotedGlobChar = false;
      $quotedGlobChar = false;
      while ($this->pos != $this->end) {
         $char = $this->look();
         if (ctype_space($char) || has_substr('|&;', $char)) {
            break;
         } elseif (has_substr('><', $char)) {
            // This is an annoying case; we treat '2>' as a single token so
            // we don't have to track whitespace tokens.
            // If the parse string isn't an integer, do the usual thing.
            if (!ctype_digit($str)) {
               break;
            }
            // Otherwise, lex the operator and convert to a redirection
            // token.
            $num = intval($str);
            $token = $this->lexOneToken();
            assert(is_array($token) && count($token) == 1);
            return [$token[0], $num];
         } elseif ($char == '\'' || $char == '"') {
            $this->eat();
            $quotedArg = $this->lexArgQuoted($char);
            if (has_substr($quotedArg, '*') || has_substr($quotedArg, '?')) {
               $quotedGlobChar = true;
            }
            $str .= $quotedArg;
         } elseif (!$this->win32Escapes && $char == '\\') {
            // Outside of a string, '\\' escapes everything.
            $this->eat();
            if ($this->pos == $this->end) {
               TestLogger::warning("escape at end of quoted argument in: %s", $this->data);
               return $str;
            }
            $str .= $this->eat();
         } elseif (has_substr('*?', $char)) {
            $unquotedGlobChar = true;
            $str .= $this->eat();
         } else {
            $str .= $this->eat();
         }
      }
      // If a quote character is present, lex_arg_quoted will remove the quotes
      // and append the argument directly.  This causes a problem when the
      // quoted portion contains a glob character, as the character will no
      // longer be treated literally.  If glob characters occur *only* inside
      // of quotes, then we can handle this by not globbing at all, and if
      // glob characters occur *only* outside of quotes, we can still glob just
      // fine.  But if a glob character occurs both inside and outside of
      // quotes this presents a problem.  In practice this is such an obscure
      // edge case that it doesn't seem worth the added complexity to support.
      // By adding an assertion, it means some bot somewhere will catch this
      // and flag the user of a non-portable test (which could almost certainly
      // be re-written to work correctly without triggering this).
      assert(!($unquotedGlobChar && $quotedGlobChar));
      if ($unquotedGlobChar) {
         return new GlobItemCommand($str);
      }
      return $str;
   }

   private function lexArgQuoted($delim)
   {
      $str = '';
      while ($this->pos != $this->end) {
         $char = $this->eat();
         if ($char == $delim) {
            return $str;
         } elseif ($char == '\\' && $delim == '"') {
            // Inside a '"' quoted string, '\\' only escapes the quote
            // character and backslash, otherwise it is preserved.
            if ($this->pos == $this->end) {
               throw new ValueException(sprintf('escape at end of quoted argument in: %s', $this->data));
            }
            $char = $this->eat();
            if ($char == '"') {
               $str .= '"';
            } elseif ($char == '\\') {
               $str .= '\\';
            } else {
               $str .= '\\' . $char;
            }
         } else {
            $str .= $char;
         }
      }
      throw new ValueException(sprintf('missing quote character in %s', $this->data));
   }

   private function lexArgChecked($char)
   {
      $pos = $this->pos;
      $res = $this->lexArgFast($char);
      $end = $this->pos;
      $this->pos = $pos;
      $reference = $this->lexArgSlow($char);
      if (!is_null($res)) {
         if ($res != $reference) {
            throw new ValueException(sprintf('Fast path failure: %s != %s', $res, $reference));
         }
         if ($this->pos != $end) {
            throw new ValueException(sprintf('Fast path failure: %s != %s', $this->pos, $end));
         }
      }
      return $reference;
   }

   private function lexArg($char)
   {
      $token = $this->lexArgFast($char);
      if ($token != null) {
         return $token;
      }
      return $this->lexArgSlow($char);
   }

   /**
    * Lex a single 'sh' token.
    */
   private function lexOneToken()
   {
      $char = $this->eat();
      if ($char == ';') {
         return [$char];
      }
      if ($char == '|') {
         if ($this->maybeEat('|')) {
            return ['||'];
         }
         return [$char];
      }
      if ($char == '&') {
         if ($this->maybeEat('&')) {
            return ['&&'];
         }
         if ($this->maybeEat('>')) {
            return ['&>'];
         }
         return [$char];
      }
      if ($char == '>') {
         if ($this->maybeEat('&')) {
            return ['>&'];
         }
         if ($this->maybeEat('>')) {
            return ['>>'];
         }
         return [$char];
      }
      if ($char == '<') {
         if ($this->maybeEat('&')) {
            return ['<&'];
         }
         if ($this->maybeEat('<')) {
            return ['<<'];
         }
         return [$char];
      }
      return $this->lexArg($char);
   }

   /**
    * @return string
    */
   public function getData(): string
   {
      return $this->data;
   }

   /**
    * @return int
    */
   public function getPos(): int
   {
      return $this->pos;
   }

   /**
    * @return int
    */
   public function getEnd(): int
   {
      return $this->end;
   }

   /**
    * @return bool
    */
   public function isWin32Escapes(): bool
   {
      return $this->win32Escapes;
   }
}