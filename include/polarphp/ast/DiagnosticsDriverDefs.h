//===--- DiagnosticsDriver.def - Diagnostics Text ---------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
//  This file defines driver-only diagnostics emitted in processing
//  command-line arguments and setting up compilation.
//  Each diagnostic is described using one of three kinds (error, warning, or
//  note) along with a unique identifier, category, options, and text, and is
//  followed by a signature describing the diagnostic argument kinds.
//
//===----------------------------------------------------------------------===//

#if !(defined(DIAG) || (defined(ERROR) && defined(WARNING) && defined(NOTE)))
#  error Must define either DIAG or the set {ERROR,WARNING,NOTE}
#endif

#ifndef DIAG
# define DIAG(ERROR,ID,Options,Text,Signature)
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

#ifndef REMARK
#  define REMARK(ID,Options,Text,Signature) \
DIAG(REMARK,ID,Options,Text,Signature)
#endif

WARNING(warning_parallel_execution_not_supported,NoneType,
        "parallel execution not supported; falling back to serial execution",
        ())

ERROR(error_unable_to_execute_command, NoneType,
      "unable to execute command: %0", (StringRef))
ERROR(error_command_signalled_without_signal_number, NoneType,
      "%0 command failed due to signal (use -v to see invocation)", (StringRef))
ERROR(error_command_signalled, NoneType,
      "%0 command failed due to signal %1 (use -v to see invocation)", (StringRef, int))
ERROR(error_command_failed, NoneType,
      "%0 command failed with exit code %1 (use -v to see invocation)",
      (StringRef, int))

ERROR(error_expected_one_frontend_job, NoneType,
      "unable to handle compilation, expected exactly one frontend job", ())
ERROR(error_expected_frontend_command, NoneType,
      "expected a swift frontend command", ())

ERROR(error_cannot_specify__o_for_multiple_outputs, NoneType,
      "cannot specify -o when generating multiple output files", ())

ERROR(error_unable_to_load_output_file_map, NoneType,
      "unable to load output file map '%1': %0", (StringRef, StringRef))

ERROR(error_no_output_file_map_specified, NoneType,
      "no output file map specified", ())

ERROR(error_unable_to_make_temporary_file, NoneType,
      "unable to make temporary file: %0", (StringRef))

ERROR(error_no_input_files, NoneType,
      "no input files", ())

ERROR(error_unexpected_input_file, NoneType,
      "unexpected input file: %0", (StringRef))

ERROR(error_unknown_target, NoneType,
      "unknown target '%0'", (StringRef))

ERROR(error_framework_bridging_header, NoneType,
      "using bridging headers with framework targets is unsupported", ())
ERROR(error_bridging_header_parseable_interface, NoneType,
      "using bridging headers with parseable module interfaces is unsupported",
      ())

ERROR(error_i_mode,NoneType,
      "the flag '-i' is no longer required and has been removed; "
      "use '%0 input-filename'", (StringRef))
WARNING(warning_unnecessary_repl_mode,NoneType,
        "unnecessary option '%0'; this is the default for '%1' "
        "with no input files", (StringRef, StringRef))
ERROR(error_unsupported_option, NoneType,
      "option '%0' is not supported by '%1'; did you mean to use '%2'?",
      (StringRef, StringRef, StringRef))

WARNING(incremental_requires_output_file_map,NoneType,
        "ignoring -incremental (currently requires an output file map)", ())
WARNING(incremental_requires_build_record_entry, NoneType,
        "ignoring -incremental; output file map has no master dependencies "
        "entry (\"%0\" under \"\")", (StringRef))

ERROR(error_os_minimum_deployment, NoneType,
      "Swift requires a minimum deployment target of %0", (StringRef))
ERROR(error_sdk_too_old,NoneType,
      "Swift does not support the SDK '%0'", (StringRef))
ERROR(error_ios_maximum_deployment_32, NoneType,
      "iOS %0 does not support 32-bit programs", (unsigned))

ERROR(error_two_files_same_name, NoneType,
      "filename \"%0\" used twice: '%1' and '%2'",
      (StringRef, StringRef, StringRef))
NOTE(note_explain_two_files_same_name, NoneType,
     "filenames are used to distinguish private declarations with the same "
     "name", ())

WARNING(warn_cannot_stat_input,NoneType,
        "unable to determine when '%0' was last modified: %1",
        (StringRef, StringRef))

WARNING(warn_unable_to_load_dependencies, NoneType,
        "unable to load dependencies file \"%0\", disabling incremental mode",
        (StringRef))

ERROR(error_input_changed_during_build,NoneType,
      "input file '%0' was modified during the build",
      (StringRef))

ERROR(error_conflicting_options, NoneType,
      "conflicting options '%0' and '%1'",
      (StringRef, StringRef))
ERROR(error_option_not_supported, NoneType,
      "'%0' is not supported with '%1'",
      (StringRef, StringRef))

WARNING(warn_ignore_embed_bitcode, NoneType,
        "ignoring -embed-bitcode since no object file is being generated", ())
WARNING(warn_ignore_embed_bitcode_marker, NoneType,
        "ignoring -embed-bitcode-marker since no object file is being generated", ())

WARNING(verify_debug_info_requires_debug_option, NoneType,
        "ignoring '-verify-debug-info'; no debug info is being generated", ())

ERROR(error_profile_missing,NoneType,
      "no profdata file exists at '%0'", (StringRef))

WARNING(warn_opt_remark_disabled, NoneType,
        "Emission of optimization records has been disabled, because it "
        "requires a single compiler invocation: consider enabling the "
        "-whole-module-optimization flag", ())

WARNING(warn_ignoring_batch_mode, NoneType,
"ignoring '-enable-batch-mode' because '%0' was also specified", (StringRef))

WARNING(warn_use_filelists_deprecated, NoneType,
        "the option '-driver-use-filelists' is deprecated; use "
        "'-driver-filelist-threshold=0' instead", ())

ERROR(cannot_find_migration_script, NoneType,
      "missing migration script from path '%0'", (StringRef))

#ifndef DIAG_NO_UNDEF
# if defined(DIAG)
#  undef DIAG
# endif
# undef NOTE
# undef WARNING
# undef ERROR
#endif
