#ifndef EXAMPLES_DELEGATION_DEMO_SHARED_DELEGATION_CRYPTO_H_
#define EXAMPLES_DELEGATION_DEMO_SHARED_DELEGATION_CRYPTO_H_

#include <string>
#include <vector>

#include "examples/delegation_demo/shared/types.h"

namespace proofs {

// 生成策略的规范化 JSON 字符串（键按字母序，紧凑格式），用于签名
// 输出示例：{"agent_id":"x","allowed_claims":["age_over_18"],"created":"...","expires":"..."}
std::string CanonicalPolicyJson(const Policy& policy);

// 计算委托消息摘要：SHA256(pk_ag_x_bytes || pk_ag_y_bytes || canonical_policy_bytes)
// agent_pkx_hex, agent_pky_hex: 0x 前缀的 64 字符 hex（来自 GenerateP256KeyPair 输出）
// out_msg_hex: 输出 0x 前缀的 64 字符 hex（32 字节 SHA256）
bool ComputeDelegationMsg(const std::string& agent_pkx_hex,
                          const std::string& agent_pky_hex,
                          const Policy& policy,
                          std::string* out_msg_hex,
                          std::string* err);

// 用 device_sk 对委托消息签名（委托消息必须是 SHA256 摘要）
// sk_hex: 0x 前缀的 64 字符 hex
// msg_hex: 0x 前缀的 64 字符 hex（32 字节摘要）
// out_sig_hex: 输出 0x 前缀的 128 字符 hex（64 字节 r||s）
bool SignDelegation(const std::string& sk_hex,
                    const std::string& msg_hex,
                    std::string* out_sig_hex,
                    std::string* err);

// 验证委托签名（供模块 B 自测使用）
bool VerifyDelegationSig(const std::string& pkx_hex,
                         const std::string& pky_hex,
                         const std::string& msg_hex,
                         const std::string& sig_hex,
                         std::string* err);

// 检查 claim alias 是否在策略允许范围内（约束⑧应用层实现）
bool PolicyAllowsClaim(const Policy& policy, const std::string& alias);

// 检查策略是否已过期（约束⑨应用层实现）
// now_iso8601: 当前时间 ISO 8601 字符串
// 策略过期 <=> policy.expires <= now
bool PolicyExpired(const Policy& policy, const std::string& now_iso8601);

}  // namespace proofs

#endif  // EXAMPLES_DELEGATION_DEMO_SHARED_DELEGATION_CRYPTO_H_
