#include "cp2-system.h"
#include "target.h"
#include "opts.h"
#include "flags.h"
#include "stringpool.h"
#include "c-family/c-common.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "common/common-target.h"
#include "cp-objcp-common.h"

enum c_language_kind c_language = clk_cxx2;

/* Implement c-family hook to add language-specific features
   for __has_{feature,extension}.  */

void
c_family_register_lang_features ()
{
  cp_register_features ();
}

/* The following function does something real, but only in Objective-C++.  */

tree
objcp_tsubst_expr (tree /*t*/, tree /*args*/, tsubst_flags_t /*complain*/,
		   tree /*in_decl*/)
{
  return NULL_TREE;
}

/* Lang hooks that are shared between C++ and ObjC++ are defined here.  Hooks
   specific to C++ or ObjC++ go in cp/cp-lang.cc and objcp/objcp-lang.cc,
   respectively.  */

/* Language hooks.  */

void
cxx2_langhook_parse_file (void)
{
  const char *foo = "*****************************************************\n";
  std::cout << foo << foo << foo << "Hello from g++2 using iostreams\n"
	    << foo << foo << foo;
}

void
cxx2_langhook_finish (void)
{
  const char *foo = "*****************************************************\n";
  std::cout << foo << foo << foo << "Hello from g++2 finish\n"
	    << foo << foo << foo;
}

static void
set_std_flags ();

void
cxx2_langhooks_init_options (unsigned int /* decoded_options_count */,
			     struct cl_decoded_option * /* decoded_options */)
{
  set_std_flags ();
}

#undef LANG_HOOKS_NAME
#undef LANG_HOOKS_PARSE_FILE
#undef LANG_HOOKS_FINISH
#undef LANG_HOOKS_INIT_OPTIONS
#undef LANG_HOOKS_POST_COMPILATION_PARSING_CLEANUPS

#define LANG_HOOKS_NAME "GNU C++2"
#define LANG_HOOKS_PARSE_FILE cxx2_langhook_parse_file
#define LANG_HOOKS_FINISH cxx2_langhook_finish
#define LANG_HOOKS_INIT_OPTIONS cxx2_langhooks_init_options
#define LANG_HOOKS_POST_COMPILATION_PARSING_CLEANUPS NULL

/* These are defined in cp-objc-common.h but not actually used,
   because they have been removed from gcc. */

#undef LANG_HOOKS_CLEAR_BINDING_STACK
#undef LANG_HOOKS_HANDLE_FILENAME

/* Below are the langhooks that are "borrowed" from cp: */

const cp_trait cp2_traits[] = {
#define DEFTRAIT(TCC, CODE, NAME, ARITY)                                       \
  {NAME, CPTK_##CODE, ARITY, (TCC == tcc_type)},
#include "cp-trait.def"
#undef DEFTRAIT
};

/* The trait table cannot have more than 255 (addr_space_t) entries since
   the index is retrieved through IDENTIFIER_CP_INDEX.  */
static_assert (ARRAY_SIZE (cp2_traits) <= 255,
	       "cp2_traits array cannot have more than 255 entries");

static enum classify_record
cp_classify_record (tree type)
{
  if (TYPE_LANG_SPECIFIC (type) && CLASSTYPE_DECLARED_CLASS (type))
    return RECORD_IS_CLASS;

  return RECORD_IS_STRUCT;
}

static const char *
cxx_dwarf_name (tree t, int verbosity)
{
  gcc_assert (DECL_P (t));

  if (DECL_NAME (t) && IDENTIFIER_ANON_P (DECL_NAME (t)))
    return NULL;
  if (verbosity >= 2)
    return decl_as_dwarf_string (t, TFF_DECL_SPECIFIERS | TFF_UNQUALIFIED_NAME
				      | TFF_NO_OMIT_DEFAULT_TEMPLATE_ARGUMENTS);

  return lang_decl_dwarf_name (t, verbosity, false);
}

/* Pruned version of c_common_post_options from c-family/c-opts.cc.

   Should return true to indicate that a compiler back-end is
   not required, such as with the -E option. */
bool
cxx2_langhooks_post_options (const char ** /* pfilename */)
{
  if (num_in_fnames != 1)
    {
      error ("Currently supports only one source file at the same time; type "
	     "%<%s %s%> for usage",
	     progname, "--help");
      /* If errorcount is nonzero after this call the compiler exits
	     immediately and the finish hook is not called.  */
      errorcount++;
      /* Don't do any compilation or preprocessing if there is no input file. */
      return true;
    }

#ifdef C_COMMON_OVERRIDE_OPTIONS
  /* Some machines may reject certain combinations of C
     language-specific options.  */
  C_COMMON_OVERRIDE_OPTIONS;
#endif

  if (flag_excess_precision == EXCESS_PRECISION_DEFAULT)
    flag_excess_precision
      = (flag_iso ? EXCESS_PRECISION_STANDARD : EXCESS_PRECISION_FAST);

  /* ISO C restricts floating-point expression contraction to within
     source-language expressions (-ffp-contract=on, currently an alias
     for -ffp-contract=off).  */
  if (flag_iso && !c_dialect_cxx ()
      && (OPTION_SET_P (flag_fp_contract_mode) == (enum fp_contract_mode) 0)
      && flag_unsafe_math_optimizations == 0)
    flag_fp_contract_mode = FP_CONTRACT_OFF;

  if (!c_dialect_cxx ())
    {
      gcc_unreachable ();
    }

  /* If -ffreestanding, -fno-hosted or -fno-builtin then disable
     pattern recognition.  */
  if (flag_no_builtin)
    SET_OPTION_IF_UNSET (&global_options, &global_options_set,
			 flag_tree_loop_distribute_patterns, 0);

  /* -Woverlength-strings is off by default, but is enabled by -Wpedantic.
     It is never enabled in C++, as the minimum limit is not normative
     in that standard.  */
  warn_overlength_strings = 0;

  /* Wmain is enabled by default in C++ but not in C.  */
  /* Wmain is disabled by default for -ffreestanding (!flag_hosted),
     even if -Wall or -Wpedantic was given (warn_main will be 2 if set
     by -Wall, 1 if set by -Wmain).  */
  if (warn_main == -1)
    warn_main = (c_dialect_cxx () && flag_hosted) ? 1 : 0;
  else if (warn_main == 2)
    warn_main = flag_hosted ? 1 : 0;

  /* In C, -Wall and -Wc++-compat enable -Wenum-compare; if it has not
     yet been set, it is disabled by default.  In C++, it is enabled
     by default.  */
  warn_enum_compare = c_dialect_cxx ();

  /* -Wpacked-bitfield-compat is on by default for the C languages.  The
     warning is issued in stor-layout.cc which is not part of the front-end so
     we need to selectively turn it on here.  */
  if (warn_packed_bitfield_compat == -1)
    warn_packed_bitfield_compat = 1;

  /* Special format checking options don't work without -Wformat; warn if
     they are used.  */
  if (!warn_format)
    {
      warning (OPT_Wformat_y2k,
	       "%<-Wformat-y2k%> ignored without %<-Wformat%>");
      warning (OPT_Wformat_extra_args,
	       "%<-Wformat-extra-args%> ignored without %<-Wformat%>");
      warning (OPT_Wformat_zero_length,
	       "%<-Wformat-zero-length%> ignored without %<-Wformat%>");
      warning (OPT_Wformat_nonliteral,
	       "%<-Wformat-nonliteral%> ignored without %<-Wformat%>");
      warning (OPT_Wformat_contains_nul,
	       "%<-Wformat-contains-nul%> ignored without %<-Wformat%>");
      warning (OPT_Wformat_security,
	       "%<-Wformat-security%> ignored without %<-Wformat%>");
    }

  /* -Wshift-overflow is enabled by default in C99 and C++11 modes.  */
  if (warn_shift_overflow == -1)
    warn_shift_overflow = cxx_dialect >= cxx11;

  /* -Wshift-negative-value is enabled by -Wextra in C99 and C++11 to C++17
     modes.  */
  if (warn_shift_negative_value == -1)
    warn_shift_negative_value
      = (extra_warnings && (cxx_dialect >= cxx11) && cxx_dialect < cxx20);

  /* -Wregister is enabled by default in C++17.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set, warn_register,
		       cxx_dialect >= cxx17);

  /* -Wcomma-subscript is enabled by default in C++20.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       warn_comma_subscript,
		       cxx_dialect >= cxx23
			 || (cxx_dialect == cxx20 && warn_deprecated));

  /* -Wvolatile is enabled by default in C++20.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set, warn_volatile,
		       cxx_dialect >= cxx20 && warn_deprecated);

  /* -Wdeprecated-enum-enum-conversion is enabled by default in C++20.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       warn_deprecated_enum_enum_conv,
		       cxx_dialect >= cxx20 && warn_deprecated);

  /* -Wdeprecated-enum-float-conversion is enabled by default in C++20.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       warn_deprecated_enum_float_conv,
		       cxx_dialect >= cxx20 && warn_deprecated);

  /* -Wtemplate-id-cdtor is enabled by default in C++20.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       warn_template_id_cdtor,
		       cxx_dialect >= cxx20 || warn_cxx20_compat);

  /* Declone C++ 'structors if -Os.  */
  if (flag_declone_ctor_dtor == -1)
    flag_declone_ctor_dtor = optimize_size;

  if (flag_abi_compat_version == 1)
    {
      warning (0, "%<-fabi-compat-version=1%> is not supported, using =2");
      flag_abi_compat_version = 2;
    }

  /* Change flag_abi_version to be the actual current ABI level, for the
     benefit of c_cpp_builtins, and to make comparison simpler.  */
  const int latest_abi_version = 19;
  /* Generate compatibility aliases for ABI v13 (8.2) by default.  */
  const int abi_compat_default = 13;

#define clamp(X)                                                               \
  if (X == 0 || X > latest_abi_version)                                        \
  X = latest_abi_version
  clamp (flag_abi_version);
  clamp (warn_abi_version);
  clamp (flag_abi_compat_version);
#undef clamp

  /* Default -Wabi= or -fabi-compat-version= from each other.  */
  if (warn_abi_version == -1 && flag_abi_compat_version != -1)
    warn_abi_version = flag_abi_compat_version;
  else if (flag_abi_compat_version == -1 && warn_abi_version != -1)
    flag_abi_compat_version = warn_abi_version;
  else if (warn_abi_version == -1 && flag_abi_compat_version == -1)
    {
      warn_abi_version = latest_abi_version;
      if (flag_abi_version == latest_abi_version)
	{
	  auto_diagnostic_group d;
	  if (warning (OPT_Wabi, "%<-Wabi%> won%'t warn about anything"))
	    {
	      inform (input_location,
		      "%<-Wabi%> warns about differences "
		      "from the most up-to-date ABI, which is also used "
		      "by default");
	      inform (input_location, "use e.g. %<-Wabi=11%> to warn about "
				      "changes from GCC 7");
	    }
	  flag_abi_compat_version = abi_compat_default;
	}
      else
	flag_abi_compat_version = latest_abi_version;
    }

  /* By default, enable the new inheriting constructor semantics along with ABI
     11.  New and old should coexist fine, but it is a change in what
     artificial symbols are generated.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       flag_new_inheriting_ctors, abi_version_at_least (11));

  /* For GCC 7, only enable DR150 resolution by default if -std=c++17.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set, flag_new_ttp,
		       cxx_dialect >= cxx17);

  /* C++11 guarantees forward progress.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set, flag_finite_loops,
		       optimize >= 2 && cxx_dialect >= cxx11);

  /* It's OK to discard calls to pure/const functions that might throw.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       flag_delete_dead_exceptions, true);

  if (cxx_dialect >= cxx11)
    {
      /* If we're allowing C++0x constructs, don't warn about C++98
	 identifiers which are keywords in C++0x.  */
      warn_cxx11_compat = 0;

      if (warn_narrowing == -1)
	warn_narrowing = 1;
    }

  else if (warn_narrowing == -1)
    warn_narrowing = 0;

  /* C++17 has stricter evaluation order requirements; let's use some of them
     for earlier C++ as well, so chaining works as expected.  */
  if (c_dialect_cxx () && flag_strong_eval_order == -1)
    flag_strong_eval_order = (cxx_dialect >= cxx17 ? 2 : 1);

  if (flag_implicit_constexpr && cxx_dialect < cxx14)
    flag_implicit_constexpr = false;

  /* Global sized deallocation is new in C++14.  */
  if (flag_sized_deallocation == -1)
    flag_sized_deallocation = (cxx_dialect >= cxx14);

  /* Pedwarn about invalid constexpr functions before C++23.  */
  if (warn_invalid_constexpr == -1)
    warn_invalid_constexpr = (cxx_dialect < cxx23);

  /* char8_t support is implicitly enabled alwyas not just in C++20 and C23.  */
  flag_char8_t = 1;

  if (flag_extern_tls_init)
    {
      if (!TARGET_SUPPORTS_ALIASES || !SUPPORTS_WEAK)
	{
	  /* Lazy TLS initialization for a variable in another TU requires
	     alias and weak reference support.  */
	  if (flag_extern_tls_init > 0)
	    sorry ("external TLS initialization functions not supported "
		   "on this target");

	  flag_extern_tls_init = 0;
	}
      else
	flag_extern_tls_init = 1;
    }

  /* Enable by default only for C++ and C++ with ObjC extensions.  */
  if (warn_return_type == -1 && c_dialect_cxx ())
    warn_return_type = 1;

  /* C++20 is the final version of concepts. We still use -fconcepts
     to know when concepts are enabled. Note that -fconcepts-ts can
     be used to include additional features, although modified to
     work with the standard.  */
  if (cxx_dialect >= cxx20)
    flag_concepts = 1;

  /* -fimmediate-escalation has no effect when immediate functions are not
     supported.  */
  if (flag_immediate_escalation && cxx_dialect < cxx20)
    flag_immediate_escalation = 0;

  /* For C++23 and explicit -finput-charset=UTF-8, turn on -Winvalid-utf8
     by default and make it a pedwarn unless -Wno-invalid-utf8.  */
  if (cxx_dialect >= cxx23)
    {
      global_options.x_warn_invalid_utf8 = 1;
    }

  input_location = UNKNOWN_LOCATION;

  return false;
}

/* Initialize the C++ traits.  */
static void
init_cp2_traits (void)
{
  tree id;

  for (unsigned int i = 0; i < ARRAY_SIZE (cp2_traits); ++i)
    {
      id = get_identifier (cp2_traits[i].name);
      IDENTIFIER_CP_INDEX (id) = cp2_traits[i].kind;
      set_identifier_kind (id, cik_trait);
    }

  /* An alias for __is_same.  */
  id = get_identifier ("__is_same_as");
  IDENTIFIER_CP_INDEX (id) = CPTK_IS_SAME;
  set_identifier_kind (id, cik_trait);
}

/* Create and tag the internal operator name for the overloaded
   operator PTR describes.  */

static tree
set_operator_ident (ovl_op_info_t *ptr)
{
  char buffer[32];
  size_t len = snprintf (
    buffer, sizeof (buffer), "operator%s%s",
    &" "[ptr->name[0] && ptr->name[0] != '_' && !ISALPHA (ptr->name[0])],
    ptr -> name);
  gcc_checking_assert (len < sizeof (buffer));

  tree ident = get_identifier_with_length (buffer, len);
  ptr->identifier = ident;

  return ident;
}

static void
init_operators (void)
{
  /* We rely on both these being zero.  */
  gcc_checking_assert (!OVL_OP_ERROR_MARK && !ERROR_MARK);

  /* This loop iterates backwards because we need to move the
     assignment operators down to their correct slots.  I.e. morally
     equivalent to an overlapping memmove where dest > src.  Slot
     zero is for error_mark, so hae no operator. */
  for (unsigned ix = OVL_OP_MAX; --ix;)
    {
      ovl_op_info_t *op_ptr = &ovl_op_info[false][ix];

      if (op_ptr->name)
	{
	  tree ident = set_operator_ident (op_ptr);
	  if (unsigned index = IDENTIFIER_CP_INDEX (ident))
	    {
	      ovl_op_info_t *bin_ptr = &ovl_op_info[false][index];

	      /* They should only differ in unary/binary ness.  */
	      gcc_checking_assert ((op_ptr->flags ^ bin_ptr->flags)
				   == OVL_OP_FLAG_AMBIARY);
	      bin_ptr->flags |= op_ptr->flags;
	      ovl_op_alternate[index] = ix;
	    }
	  else
	    {
	      IDENTIFIER_CP_INDEX (ident) = ix;
	      set_identifier_kind (ident, cik_simple_op);
	    }
	}
      if (op_ptr->tree_code)
	{
	  gcc_checking_assert (op_ptr->ovl_op_code == ix
			       && !ovl_op_mapping[op_ptr->tree_code]);
	  ovl_op_mapping[op_ptr->tree_code] = op_ptr->ovl_op_code;
	}

      ovl_op_info_t *as_ptr = &ovl_op_info[true][ix];
      if (as_ptr->name)
	{
	  /* These will be placed at the start of the array, move to
	     the correct slot and initialize.  */
	  if (as_ptr->ovl_op_code != ix)
	    {
	      ovl_op_info_t *dst_ptr = &ovl_op_info[true][as_ptr->ovl_op_code];
	      gcc_assert (as_ptr->ovl_op_code > ix && !dst_ptr->tree_code);
	      memcpy (dst_ptr, as_ptr, sizeof (*dst_ptr));
	      memset (as_ptr, 0, sizeof (*as_ptr));
	      as_ptr = dst_ptr;
	    }

	  tree ident = set_operator_ident (as_ptr);
	  gcc_checking_assert (!IDENTIFIER_CP_INDEX (ident));
	  IDENTIFIER_CP_INDEX (ident) = as_ptr->ovl_op_code;
	  set_identifier_kind (ident, cik_assign_op);

	  gcc_checking_assert (
	    !ovl_op_mapping[as_ptr->tree_code]
	    || (ovl_op_mapping[as_ptr->tree_code] == as_ptr->ovl_op_code));
	  ovl_op_mapping[as_ptr->tree_code] = as_ptr->ovl_op_code;
	}
    }
}

/* Pruned version of cxx_init */
bool
cxx2_langhooks_init (void)
{
  location_t saved_loc;
  unsigned int i;
  static const enum tree_code stmt_codes[]
    = {CTOR_INITIALIZER, TRY_BLOCK, HANDLER,	  EH_SPEC_BLOCK, USING_STMT,
       TAG_DEFN,	 IF_STMT,   CLEANUP_STMT, FOR_STMT,	 RANGE_FOR_STMT,
       WHILE_STMT,	 DO_STMT,   BREAK_STMT,	  CONTINUE_STMT, SWITCH_STMT,
       EXPR_STMT,	 OMP_DEPOBJ};

  memset (&statement_code_p, 0, sizeof (statement_code_p));
  for (i = 0; i < ARRAY_SIZE (stmt_codes); i++)
    statement_code_p[stmt_codes[i]] = true;

  saved_loc = input_location;
  input_location = BUILTINS_LOCATION;

  init_reswords ();
  init_cp2_traits ();
  init_tree ();
  init_cp_semantics ();
  init_operators ();
  init_method ();

  current_function_decl = NULL;
  class_type_node = ridpointers[(int) RID_CLASS];

  cxx_init_decl_processing ();

  input_location = saved_loc;

  return true;
}

#undef LANG_HOOKS_INIT
#undef LANG_HOOKS_CLASSIFY_RECORD
#undef LANG_HOOKS_INIT_TS
#undef LANG_HOOKS_DWARF_NAME
#undef LANG_HOOKS_POST_OPTIONS

#define LANG_HOOKS_INIT cxx2_langhooks_init
#define LANG_HOOKS_CLASSIFY_RECORD cp_classify_record
#define LANG_HOOKS_INIT_TS cp_common_init_ts
#define LANG_HOOKS_DWARF_NAME cxx_dwarf_name
#define LANG_HOOKS_POST_OPTIONS cxx2_langhooks_post_options

/* Construct the lang_hooks */

struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

static void
set_std_flags ()
{
  flag_no_gnu_keywords = 0;
  flag_no_nonansi_builtin = 0;
  flag_iso = 0;
  /* C++23 includes the C11 standard library.  */
  flag_isoc94 = 1;
  flag_isoc99 = 1;
  flag_isoc11 = 1;
  // flag_coroutines = true;
  cxx_dialect = cxx23;
}

// This is not generated, as there is no GTY (()) markers in this file(?).
// #include "gt-cp2-cp2-lang.h"
#include "gtype-cp2.h"
