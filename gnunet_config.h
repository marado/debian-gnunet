/* gnunet_config.h.  Generated from gnunet_config.h.in by configure.  */
/* gnunet_config.h.in.  Generated from configure.ac by autoheader.  */

#define _GNU_SOURCE  1

/* Using texinfo version 4 specific switches */
/* #undef ACTIVATE_TEXINFO4 */

/* Define to 1 if the `closedir' function returns void instead of `int'. */
/* #undef CLOSEDIR_VOID */

/* This is an Apple Darwin system */
/* #undef DARWIN */

/* Not building the documentation */
#define DOCUMENTATION 1

/* Doing a normal build, more than only documentation */
#define DOCUMENTATION_ONLY 0

/* Benchmarks are enabled */
/* #undef ENABLE_BENCHMARK */

/* enable expensive heap statistics */
#define ENABLE_HEAP_STATISTICS 0

/* enable compilation of malicious code */
#define ENABLE_MALICIOUS 0

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* have NSE send timestamp information to testbed logger */
#define ENABLE_NSE_HISTOGRAM 0

/* 1 if freed memory should be poisoned, 0 otherwise */
#define ENABLE_POISONING 0

/* Build with support for SuperMUC */
#define ENABLE_SUPERMUC 0

/* Build a Mac OS X Framework */
/* #undef FRAMEWORK_BUILD */

/* This is a GNU system */
/* #undef GNU */

/* Define to cull all logging calls */
/* #undef GNUNET_CULL_LOGGING */

/* This should be the default choice for the name of the first network
   interface */
#define GNUNET_DEFAULT_INTERFACE "eth0"

/* 1 if extra logging is enabled, 2 for very verbose extra logging, 0
   otherwise */
#define GNUNET_EXTRA_LOGGING GNUNET_NO

/* Not installing gnunet-logread */
#define GNUNET_LOGREAD 0

/* Lacking ABE library */
#define HAVE_ABE 0

/* Define to 1 if you have the <argz.h> header file. */
/* #undef HAVE_ARGZ_H */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `atoll' function. */
#define HAVE_ATOLL 1

/* Path to awk */
#define HAVE_AWK 1

/* Define to 1 if you have the <bluetooth/bluetooth.h> header file. */
/* #undef HAVE_BLUETOOTH_BLUETOOTH_H */

/* Define to 1 if you have the <byteswap.h> header file. */
#define HAVE_BYTESWAP_H 1

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define to 1 if your system has a working `chown' function. */
#define HAVE_CHOWN 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the declaration of `CURLINFO_TLS_SESSION', and to 0
   if you don't. */
/* #undef HAVE_DECL_CURLINFO_TLS_SESSION */

/* Define to 1 if you have the declaration of `gcry_mpi_set_opaque_copy', and
   to 0 if you don't. */
#define HAVE_DECL_GCRY_MPI_SET_OPAQUE_COPY 1

/* Define to 1 if you have the declaration of `struct in6_ifreq', and to 0 if
   you don't. */
#define HAVE_DECL_STRUCT_IN6_IFREQ 0

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
/* #undef HAVE_EXECINFO_H */

/* explicit_bzero supported */
/* #undef HAVE_EXPLICIT_BZERO */

/* Define to 1 if you have the <extractor.h> header file. */
#define HAVE_EXTRACTOR_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `freeifaddrs' function. */
#define HAVE_FREEIFADDRS 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* getaddrinfo_a supported */
/* #undef HAVE_GETADDRINFO_A */

/* Define to 1 if you have the `gethostbyname2' function. */
#define HAVE_GETHOSTBYNAME2 1

/* Define to 1 if you have the `getifaddrs' function. */
#define HAVE_GETIFADDRS 1

/* getloadavg supported */
#define HAVE_GETLOADAVG 1

/* Define to 1 if you have the `getpeereid' function. */
/* #undef HAVE_GETPEEREID */

/* Define to 1 if you have the `getpeerucred' function. */
/* #undef HAVE_GETPEERUCRED */

/* Define to 1 if you have the `getresgid' function. */
#define HAVE_GETRESGID 1

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `getrusage' function. */
#define HAVE_GETRUSAGE 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* We have GnuTLS */
#define HAVE_GNUTLS true

/* Define to 1 if you have the <gnutls/abstract.h> header file. */
#define HAVE_GNUTLS_ABSTRACT_H 1

/* We have GnuTLS with DANE support */
#define HAVE_GNUTLS_DANE 0

/* Define to 1 if you have the <gnutls/dane.h> header file. */
/* #undef HAVE_GNUTLS_DANE_H */

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <idn2.h> header file. */
/* #undef HAVE_IDN2_H */

/* Define to 1 if you have the <idn2/idn2.h> header file. */
/* #undef HAVE_IDN2_IDN2_H */

/* Define to 1 if you have the <idna.h> header file. */
#define HAVE_IDNA_H 1

/* Define to 1 if you have the <idn/idna.h> header file. */
/* #undef HAVE_IDN_IDNA_H */

/* Define to 1 if you have the <ifaddrs.h> header file. */
#define HAVE_IFADDRS_H 1

/* Define to 1 if you have the `initgroups' function. */
#define HAVE_INITGROUPS 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Lacking jansson library */
#define HAVE_JANSSON 1

/* Define to 1 if you have the <jansson.h> header file. */
/* #undef HAVE_JANSSON_H */

/* Define to 1 if you have the <kstat.h> header file. */
/* #undef HAVE_KSTAT_H */

/* Define to 1 if you have the <kvm.h> header file. */
/* #undef HAVE_KVM_H */

/* Define to 1 if you have the <langinfo.h> header file. */
#define HAVE_LANGINFO_H 1

/* external libatomic */
#define HAVE_LIBATOMIC 1

/* Lacking bluetooth library */
#define HAVE_LIBBLUETOOTH 0

/* Lacking libcurl */
#define HAVE_LIBCURL 0

/* Define to 1 if you have the `curl-gnutls' library (-lcurl-gnutls). */
/* #undef HAVE_LIBCURL_GNUTLS */

/* Have libextractor */
#define HAVE_LIBEXTRACTOR 1

/* Lacking libgnurl */
#define HAVE_LIBGNURL 1

/* Define to 1 if you have 'libidn' (-lidn). */
#define HAVE_LIBIDN 1

/* Define to 1 if you have 'libidn2' (-lidn2). */
/* #undef HAVE_LIBIDN2 */

/* Define to 1 if you have the `intl' library (-lintl). */
/* #undef HAVE_LIBINTL */

/* Define to 1 if you have the <libintl.h> header file. */
#define HAVE_LIBINTL_H 1

/* Define to 1 if you have the `kstat' library (-lkstat). */
/* #undef HAVE_LIBKSTAT */

/* Define to 1 if you have the `kvm' library (-lkvm). */
/* #undef HAVE_LIBKVM */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the <libpq-fe.h> header file. */
/* #undef HAVE_LIBPQ_FE_H */

/* Define to 1 if you have the `resolv' library (-lresolv). */
/* #undef HAVE_LIBRESOLV */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the libunistring library. */
#define HAVE_LIBUNISTRING 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <ltdl.h> header file. */
#define HAVE_LTDL_H 1

/* Define to 1 if you have the <mach/mach.h> header file. */
/* #undef HAVE_MACH_MACH_H */

/* Define to 1 if you have the `mallinfo' function. */
/* #undef HAVE_MALLINFO */

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <malloc/malloc.h> header file. */
/* #undef HAVE_MALLOC_MALLOC_H */

/* Define to 1 if you have the <malloc/malloc_np.h> header file. */
/* #undef HAVE_MALLOC_MALLOC_NP_H */

/* Define to 1 if you have the `malloc_size' function. */
/* #undef HAVE_MALLOC_SIZE */

/* Define to 1 if you have the `malloc_usable_size' function. */
#define HAVE_MALLOC_USABLE_SIZE 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* memrchr supported */
#define HAVE_MEMRCHR 1

/* memset_s supported */
/* #undef HAVE_MEMSET_S */

/* We have libmicrohttpd */
#define HAVE_MHD 1

/* Define to 1 if you have the `mremap' function. */
#define HAVE_MREMAP 1

/* Define to 1 if you have the <mysql/mysql.h> header file. */
/* #undef HAVE_MYSQL_MYSQL_H */

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
#define HAVE_NETINET_IN_SYSTM_H 1

/* Define to 1 if you have the <netinet/ip.h> header file. */
#define HAVE_NETINET_IP_H 1

/* Define to 1 if you have the <nss.h> header file. */
/* #undef HAVE_NSS_H */

/* Define to 1 if the system has the type `off_t'. */
#define HAVE_OFF_T 1

/* Lacking ogg */
/* #undef HAVE_OGG */

/* Define to 1 if you have the <ogg/ogg.h> header file. */
/* #undef HAVE_OGG_OGG_H */

/* Lacking libopus library */
#define HAVE_OPUS 0

/* Define to 1 if you have the <opus/opus.h> header file. */
/* #undef HAVE_OPUS_OPUS_H */

/* Lacking pbc library */
#define HAVE_PBC 0

/* Path to Perl */
#define HAVE_PERL 1

/* Lacking PostgreSQL */
#define HAVE_POSTGRESQL 0

/* Lacking libpulse(audio) library */
#define HAVE_PULSE 0

/* Define to 1 if you have the <pulse/simple.h> header file. */
/* #undef HAVE_PULSE_SIMPLE_H */

/* Define to 1 if you have the `random' function. */
#define HAVE_RANDOM 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if the system has the type `sigset_t'. */
#define HAVE_SIGSET_T 1

/* Define to 1 if the system has the type `size_t'. */
#define HAVE_SIZE_T 1

/* Do we have sockaddr_in.sin_len? */
/* #undef HAVE_SOCKADDR_IN_SIN_LEN */

/* Do we have sockaddr_un.sun_len? */
/* #undef HAVE_SOCKADDR_UN_SUN_LEN */

/* Define to 1 if you have the <sockLib.h> header file. */
/* #undef HAVE_SOCKLIB_H */

/* Define to 1 if you have the <sqlite3.h> header file. */
#define HAVE_SQLITE3_H 1

/* Define to 1 if you have the `srandom' function. */
#define HAVE_SRANDOM 1

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define to 1 if you have the `stat64' function. */
#define HAVE_STAT64 1

/* Define to 1 if you have the `statfs' function. */
#define HAVE_STATFS 1

/* Define to 1 if you have the `statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdatomic.h> header file. */
#define HAVE_STDATOMIC_H 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strndup' function. */
#define HAVE_STRNDUP 1

/* Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/* Define to 1 if you have the `sysconf' function. */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/endian.h> header file. */
/* #undef HAVE_SYS_ENDIAN_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/msg.h> header file. */
#define HAVE_SYS_MSG_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#define HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysinfo.h> header file. */
#define HAVE_SYS_SYSINFO_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/ucred.h> header file. */
/* #undef HAVE_SYS_UCRED_H */

/* Define to 1 if you have the <sys/vfs.h> header file. */
#define HAVE_SYS_VFS_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define this if __thread is supported */
#define HAVE_THREAD_LOCAL_GCC 1

/* Define if struct tm has the tm_gmtoff member. */
#define HAVE_TM_GMTOFF 1

/* Define to 1 if you have the <ucred.h> header file. */
/* #undef HAVE_UCRED_H */

/* We can access-64 bit values that are only 32-bit aligned */
#define HAVE_UNALIGNED_64_ACCESS 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <unistr.h> header file. */
#define HAVE_UNISTR_H 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `wait4' function. */
#define HAVE_WAIT4 1

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Lacking zbar library */
#define HAVE_ZBAR 0

/* Define to 1 if you have the <zbar.h> header file. */
/* #undef HAVE_ZBAR_H */

/* Have compression library */
#define HAVE_ZLIB /**/

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* Path to ifconfig */
#define IFCONFIG "/sbin/ifconfig"

/* if_tun.h include path */
#define IF_TUN_HDR "linux/if_tun.h"

/* Using texinfo version 5 or later switches */
#define INCLUDE_MANPAGES 0

/* Path to ip6tables */
/* #undef IP6TABLES */

/* Path to iptables */
/* #undef IPTABLES */

/* Defined if libcurl supports AsynchDNS */
/* #undef LIBCURL_FEATURE_ASYNCHDNS */

/* Defined if libcurl supports IDN */
/* #undef LIBCURL_FEATURE_IDN */

/* Defined if libcurl supports IPv6 */
/* #undef LIBCURL_FEATURE_IPV6 */

/* Defined if libcurl supports KRB4 */
/* #undef LIBCURL_FEATURE_KRB4 */

/* Defined if libcurl supports libz */
/* #undef LIBCURL_FEATURE_LIBZ */

/* Defined if libcurl supports NTLM */
/* #undef LIBCURL_FEATURE_NTLM */

/* Defined if libcurl supports SSL */
/* #undef LIBCURL_FEATURE_SSL */

/* Defined if libcurl supports SSPI */
/* #undef LIBCURL_FEATURE_SSPI */

/* Defined if libcurl supports DICT */
/* #undef LIBCURL_PROTOCOL_DICT */

/* Defined if libcurl supports FILE */
/* #undef LIBCURL_PROTOCOL_FILE */

/* Defined if libcurl supports FTP */
/* #undef LIBCURL_PROTOCOL_FTP */

/* Defined if libcurl supports FTPS */
/* #undef LIBCURL_PROTOCOL_FTPS */

/* Defined if libcurl supports HTTP */
/* #undef LIBCURL_PROTOCOL_HTTP */

/* Defined if libcurl supports HTTPS */
/* #undef LIBCURL_PROTOCOL_HTTPS */

/* Defined if libcurl supports IMAP */
/* #undef LIBCURL_PROTOCOL_IMAP */

/* Defined if libcurl supports LDAP */
/* #undef LIBCURL_PROTOCOL_LDAP */

/* Defined if libcurl supports POP3 */
/* #undef LIBCURL_PROTOCOL_POP3 */

/* Defined if libcurl supports RTSP */
/* #undef LIBCURL_PROTOCOL_RTSP */

/* Defined if libcurl supports SMTP */
/* #undef LIBCURL_PROTOCOL_SMTP */

/* Defined if libcurl supports TELNET */
/* #undef LIBCURL_PROTOCOL_TELNET */

/* Defined if libcurl supports TFTP */
/* #undef LIBCURL_PROTOCOL_TFTP */

/* Defined if libgnurl supports AsynchDNS */
#define LIBGNURL_FEATURE_ASYNCHDNS 1

/* Defined if libgnurl supports IDN */
/* #undef LIBGNURL_FEATURE_IDN */

/* Defined if libgnurl supports IPv6 */
#define LIBGNURL_FEATURE_IPV6 1

/* Defined if libgnurl supports KRB4 */
/* #undef LIBGNURL_FEATURE_KRB4 */

/* Defined if libgnurl supports libz */
#define LIBGNURL_FEATURE_LIBZ 1

/* Defined if libgnurl supports NTLM */
#define LIBGNURL_FEATURE_NTLM 1

/* Defined if libgnurl supports SSL */
#define LIBGNURL_FEATURE_SSL 1

/* Defined if libgnurl supports SSPI */
/* #undef LIBGNURL_FEATURE_SSPI */

/* Defined if libgnurl supports DICT */
/* #undef LIBGNURL_PROTOCOL_DICT */

/* Defined if libgnurl supports FILE */
/* #undef LIBGNURL_PROTOCOL_FILE */

/* Defined if libgnurl supports FTP */
/* #undef LIBGNURL_PROTOCOL_FTP */

/* Defined if libgnurl supports FTPS */
/* #undef LIBGNURL_PROTOCOL_FTPS */

/* Defined if libgnurl supports HTTP */
#define LIBGNURL_PROTOCOL_HTTP 1

/* Defined if libgnurl supports HTTPS */
#define LIBGNURL_PROTOCOL_HTTPS 1

/* Defined if libgnurl supports IMAP */
/* #undef LIBGNURL_PROTOCOL_IMAP */

/* Defined if libgnurl supports LDAP */
/* #undef LIBGNURL_PROTOCOL_LDAP */

/* Defined if libgnurl supports POP3 */
/* #undef LIBGNURL_PROTOCOL_POP3 */

/* Defined if libgnurl supports RTSP */
/* #undef LIBGNURL_PROTOCOL_RTSP */

/* Defined if libgnurl supports SMTP */
/* #undef LIBGNURL_PROTOCOL_SMTP */

/* Defined if libgnurl supports TELNET */
/* #undef LIBGNURL_PROTOCOL_TELNET */

/* Defined if libgnurl supports TFTP */
/* #undef LIBGNURL_PROTOCOL_TFTP */

/* This is a Linux kernel */
#define LINUX 1

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* required libgcrypt version */
#define NEED_LIBGCRYPT_VERSION "1.6.0"

/* Some strange OS */
/* #undef OTHEROS */

/* Name of package */
#define PACKAGE "gnunet"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "bug-gnunet@gnu.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "gnunet"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "gnunet 0.12.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gnunet"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.12.0"

/* Path to ip */
#define PATH_TO_IP "/sbin/ip"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* This is a Solaris system */
/* #undef SOLARIS */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Path to sysctl */
#define SYSCTL "/sbin/sysctl"

/* Canonical compilation */
#define TALER_WALLET_ONLY 0

/* Not building section 7 mdoc output */
#define TEXI2MDOC_GENERATION 0

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Path to upnpc binary */
/* #undef UPNPC */

/* VCS revision/hash or tarball version */
#define VCS_VERSION "git-f0f45c511"

/* Version number of package */
#define VERSION "0.12.0"

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Need with solaris or errno doesnt work */
/* #undef _REENTRANT */

/* Define curl_free() as free() if our version of curl lacks curl_free. */
/* #undef curl_free */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
