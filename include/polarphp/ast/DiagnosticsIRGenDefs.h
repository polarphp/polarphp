//===--- DiagnosticsIRGen.def - Diagnostics Text ----------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/01.
//===----------------------------------------------------------------------===//
//  This file defines diagnostics emitted during IR generation.
//  Each diagnostic is described using one of three kinds (error, warning, or
//  note) along with a unique identifier, category, options, and text, and is
//  followed by a signature describing the diagnostic argument kinds.
//
//===----------------------------------------------------------------------===//

#if !(defined(DIAG) || (defined(ERROR) && defined(WARNING) && defined(NOTE)))
#  error Must define either DIAG or the set {ERROR,WARNING,NOTE}
#endif

#ifndef ERROR
#  define ERROR(ID,Options,Text,Signature)   \
  DIAG(ERROR,ID,Options,Text,Signature)
#endif

#ifndef WARNING
#  define WARNING(ID,Options,Text,Signature) \
  DIAG(WARNING,ID,Options,Text,Signature)
#endif

#ifndef NOTE
#  define NOTE(ID,Options,Text,Signature) \
  DIAG(NOTE,ID,Options,Text,Signature)
#endif


ERROR(no_llvm_target,NoneType,
      "error loading LLVM target for triple '%0': %1", (StringRef, StringRef))
ERROR(error_codegen_init_fail,NoneType,
      "cannot initialize code generation passes for target", ())

ERROR(irgen_unimplemented,NoneType,
      "unimplemented IR generation feature %0", (StringRef))
ERROR(irgen_failure,NoneType, "IR generation failure: %0", (StringRef))

ERROR(type_to_verify_not_found,NoneType, "unable to find type '%0' to verify",
      (StringRef))
ERROR(type_to_verify_ambiguous,NoneType, "type to verify '%0' is ambiguous",
      (StringRef))
ERROR(type_to_verify_dependent,NoneType,
      "type to verify '%0' has unbound generic parameters",
      (StringRef))
ERROR(too_few_output_filenames,NoneType,
      "too few output file names specified", ())
ERROR(no_input_files_for_mt,NoneType,
      "no swift input files for multi-threaded compilation", ())

ERROR(alignment_dynamic_type_layout_unsupported,NoneType,
      "@_alignment is not supported on types with dynamic layout", ())
ERROR(alignment_less_than_natural,NoneType,
      "@_alignment cannot decrease alignment below natural alignment of %0",
      (unsigned))
ERROR(alignment_more_than_maximum,NoneType,
      "@_alignment cannot increase alignment above maximum alignment of %0",
      (unsigned))

#ifndef DIAG_NO_UNDEF
# if defined(DIAG)
#  undef DIAG
# endif
# undef NOTE
# undef WARNING
# undef ERROR
#endif
