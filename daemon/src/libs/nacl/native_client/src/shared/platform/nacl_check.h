#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CHECK_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CHECK_H_
#define CHECK(bool_expr) do { (void)(bool_expr); } while (0)
#define DCHECK(bool_expr) do { (void)(bool_expr); } while (0)
#endif
