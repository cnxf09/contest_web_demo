"""
ZK-AgentAuth Web Demo Backend
模块 A（Issuer）和模块 B（Alice 委托）的 Flask 包装层
"""
import json
import os
import subprocess
import uuid
from datetime import datetime
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory

app = Flask(__name__, static_folder="static")

# ── 路径配置（相对 web_demo/ 的父目录 longfellow-zk/）────────────────────
BASE_DIR    = Path(__file__).resolve().parent.parent
ISSUER_BIN   = BASE_DIR / "build-delegation/examples/delegation_demo/delegation_demo_issuer"
ALICE_BIN    = BASE_DIR / "build-delegation/examples/delegation_demo/delegation_demo_alice"
AGENT_BIN    = BASE_DIR / "build-delegation/examples/delegation_demo/delegation_demo_agent"
VERIFIER_BIN = BASE_DIR / "build-delegation/examples/delegation_demo/delegation_demo_verifier"
SESSIONS_DIR = BASE_DIR / "run/sessions"

# ── 会话状态（内存）──────────────────────────────────────────────────────
# session_id -> {"dir": str, "issued": bool, "delegated": bool}
sessions: dict = {}

# ── Claim 元数据（固定 example 3）────────────────────────────────────────
CLAIM_META = [
    {"alias": "age_over_18",  "label": "年满18岁",  "value": "是",        "icon": "🔞"},
    {"alias": "family_name",  "label": "姓氏",      "value": "Mustermann","icon": "👤"},
    {"alias": "birth_date",   "label": "出生日期",  "value": "1971-09-01","icon": "📅"},
    {"alias": "height",       "label": "身高",      "value": "175 cm",    "icon": "📏"},
]


# ── 工具函数 ─────────────────────────────────────────────────────────────
def shorten(hex_str: str, chars: int = 12) -> str:
    """截断长 hex 字符串用于展示，如 0x1a2b3c…"""
    s = hex_str.strip()
    if len(s) <= chars * 2:
        return s
    return s[:chars] + "…"


# ── 路由 ─────────────────────────────────────────────────────────────────
@app.route("/")
def index():
    return send_from_directory("static", "index.html")


@app.route("/api/issue", methods=["POST"])
def issue():
    """
    POST /api/issue
    Body: { "name": "张三", "birth_date": "1998-03-15", "height": 175 }
    调用 delegation_demo_issuer issue --example 3 --out <session_dir>/issue
    """
    data = request.get_json(silent=True) or {}

    session_id = str(uuid.uuid4())
    session_dir = SESSIONS_DIR / session_id
    issue_dir   = session_dir / "issue"
    session_dir.mkdir(parents=True, exist_ok=True)

    cmd = [str(ISSUER_BIN), "issue", "--example", "3", "--out", str(issue_dir)]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "issuer timed out"}), 500
    except FileNotFoundError:
        return jsonify({"status": "error",
                        "message": f"issuer binary not found: {ISSUER_BIN}"}), 500

    if result.returncode != 0:
        return jsonify({"status": "error",
                        "message": result.stderr or result.stdout}), 500

    issuer_public_dir = issue_dir / "issuer_public"
    try:
        issuer_pkx = (issuer_public_dir / "issuer_pkx.txt").read_text().strip()
        now_str    = (issuer_public_dir / "now.txt").read_text().strip()
        doc_type   = (issuer_public_dir / "doc_type.txt").read_text().strip()
    except OSError as e:
        return jsonify({"status": "error", "message": str(e)}), 500

    sessions[session_id] = {"dir": str(session_dir), "issued": True, "delegated": False}

    # 用用户输入的姓名覆盖展示，但 ZK 证明使用 example 3 固定模板
    display_name = data.get("name", "Mustermann")
    claims = []
    for m in CLAIM_META:
        c = dict(m)
        if m["alias"] == "family_name":
            c["value"] = display_name
        elif m["alias"] == "birth_date" and data.get("birth_date"):
            c["value"] = data["birth_date"]
        elif m["alias"] == "height" and data.get("height"):
            c["value"] = f"{data['height']} cm"
        claims.append(c)

    return jsonify({
        "status": "ok",
        "session_id": session_id,
        "credential": {
            "issuer_pkx_short": shorten(issuer_pkx),
            "doc_type": doc_type,
            "issued_at": now_str,
            "claims": claims,
        },
    })


@app.route("/api/delegate", methods=["POST"])
def delegate():
    """
    POST /api/delegate
    Body: {
      "session_id": "...",
      "allowed_claims": ["age_over_18"],
      "expires": "2027-01-01T00:00:00Z",
      "agent_id": "bookstore-agent"
    }
    调用 delegation_demo_alice delegate ...
    """
    data = request.get_json(silent=True) or {}
    session_id    = data.get("session_id", "")
    allowed_claims = data.get("allowed_claims", [])
    expires       = data.get("expires", "2027-01-01T00:00:00Z")
    agent_id      = data.get("agent_id", "agent") or "agent"

    if session_id not in sessions:
        return jsonify({"status": "error", "message": "会话不存在，请先颁发凭证"}), 400
    if not allowed_claims:
        return jsonify({"status": "error", "message": "请至少选择一个委托属性"}), 400

    session_dir  = Path(sessions[session_id]["dir"])
    holder_dir   = session_dir / "issue" / "holder"
    delegation_dir = session_dir / "delegation"

    # 若已有旧 delegation 目录，删除后重新生成
    import shutil
    if delegation_dir.exists():
        shutil.rmtree(delegation_dir)

    cmd = [
        str(ALICE_BIN), "delegate",
        "--holder",   str(holder_dir),
        "--expires",  expires,
        "--agent-id", agent_id,
        "--out",      str(delegation_dir),
    ]
    for claim in allowed_claims:
        cmd += ["--claim", claim]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "alice timed out"}), 500
    except FileNotFoundError:
        return jsonify({"status": "error",
                        "message": f"alice binary not found: {ALICE_BIN}"}), 500

    if result.returncode != 0:
        return jsonify({"status": "error",
                        "message": result.stderr or result.stdout}), 500

    try:
        policy_json_str = (delegation_dir / "policy.json").read_text()
        del_msg  = (delegation_dir / "delegation_msg.txt").read_text().strip()
        del_sig  = (delegation_dir / "delegation_sig.txt").read_text().strip()
        agent_pkx = (delegation_dir / "agent_pkx.txt").read_text().strip()
        agent_pky = (delegation_dir / "agent_pky.txt").read_text().strip()
        policy   = json.loads(policy_json_str)
    except OSError as e:
        return jsonify({"status": "error", "message": str(e)}), 500

    sessions[session_id]["delegated"] = True

    # claim label 映射
    alias_to_label = {m["alias"]: m["label"] for m in CLAIM_META}

    return jsonify({
        "status": "ok",
        "delegation": {
            "agent_id":      policy["agent_id"],
            "allowed_claims": policy["allowed_claims"],
            "allowed_labels": [alias_to_label.get(a, a) for a in policy["allowed_claims"]],
            "expires":        policy["expires"],
            "created":        policy["created"],
            "agent_pkx_short": shorten(agent_pkx),
            "agent_pky_short": shorten(agent_pky),
            "agent_pkx":      agent_pkx,
            "agent_pky":      agent_pky,
            "delegation_sig_short": shorten(del_sig),
            "delegation_sig": del_sig,
            "delegation_msg": del_msg,
            "policy_json":    policy_json_str,
        },
    })


@app.route("/api/present", methods=["POST"])
def present():
    """
    POST /api/present
    Body: { "session_id": "...", "requested_claims": ["age_over_18"] }
    1. 调用 verifier request 生成验证请求
    2. 调用 agent present 生成 ZK 证明
    """
    data = request.get_json(silent=True) or {}
    session_id = data.get("session_id", "")
    requested_claims = data.get("requested_claims", [])

    if session_id not in sessions:
        return jsonify({"status": "error", "message": "会话不存在"}), 400
    if not sessions[session_id].get("delegated"):
        return jsonify({"status": "error", "message": "请先生成委托"}), 400
    if not requested_claims:
        return jsonify({"status": "error", "message": "请指定至少一个 claim"}), 400

    session_dir    = Path(sessions[session_id]["dir"])
    issuer_pub_dir = session_dir / "issue" / "issuer_public"
    delegation_dir = session_dir / "delegation"
    request_dir    = session_dir / "request"
    present_dir    = session_dir / "presentation"

    import shutil
    for d in [request_dir, present_dir]:
        if d.exists():
            shutil.rmtree(d)

    # D-1: verifier request
    cmd_req = [str(VERIFIER_BIN), "request", "--issuer-public", str(issuer_pub_dir),
               "--out", str(request_dir)]
    for c in requested_claims:
        cmd_req += ["--claim", c]

    try:
        r1 = subprocess.run(cmd_req, capture_output=True, text=True, timeout=120)
    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        return jsonify({"status": "error", "message": str(e)}), 500
    if r1.returncode != 0:
        return jsonify({"status": "error",
                        "message": "verifier request: " + (r1.stderr or r1.stdout)}), 500

    # C: agent present
    cmd_present = [str(AGENT_BIN), "present",
                   "--delegation", str(delegation_dir),
                   "--issuer-public", str(issuer_pub_dir),
                   "--request", str(request_dir),
                   "--out", str(present_dir)]

    try:
        r2 = subprocess.run(cmd_present, capture_output=True, text=True, timeout=120)
    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        return jsonify({"status": "error", "message": str(e)}), 500
    if r2.returncode != 0:
        return jsonify({"status": "error",
                        "message": "agent present: " + (r2.stderr or r2.stdout)}), 500

    sessions[session_id]["presented"] = True

    proof_path = present_dir / "proof.bin"
    proof_size = proof_path.stat().st_size if proof_path.exists() else 0
    token_exists = (present_dir / "delegation_token.json").exists()

    alias_to_label = {m["alias"]: m["label"] for m in CLAIM_META}

    return jsonify({
        "status": "ok",
        "presentation": {
            "proof_size_bytes": proof_size,
            "claims_proved":   requested_claims,
            "claims_labels":   [alias_to_label.get(a, a) for a in requested_claims],
            "has_delegation_token": token_exists,
        },
    })


@app.route("/api/verify", methods=["POST"])
def verify():
    """
    POST /api/verify
    Body: { "session_id": "..." }
    调用 verifier verify，返回结构化结果
    """
    data = request.get_json(silent=True) or {}
    session_id = data.get("session_id", "")

    if session_id not in sessions:
        return jsonify({"status": "error", "message": "会话不存在"}), 400
    if not sessions[session_id].get("presented"):
        return jsonify({"status": "error", "message": "请先生成证明"}), 400

    session_dir    = Path(sessions[session_id]["dir"])
    issuer_pub_dir = session_dir / "issue" / "issuer_public"
    request_dir    = session_dir / "request"
    present_dir    = session_dir / "presentation"

    cmd = [str(VERIFIER_BIN), "verify",
           "--issuer-public", str(issuer_pub_dir),
           "--request", str(request_dir),
           "--presentation", str(present_dir)]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        return jsonify({"status": "error", "message": str(e)}), 500

    stdout = result.stdout

    def check(label):
        for line in stdout.splitlines():
            if label in line:
                return "PASS" in line
        return False

    checks = {
        "zk_proof":        check("ZK proof"),
        "delegation_sig":  check("Delegation sig"),
        "policy_claims":   check("Policy claims"),
        "policy_expiry":   check("Policy expiry"),
    }
    overall = "ACCEPT" if result.returncode == 0 else "REJECT"

    # 读 delegation_token 获取 policy 信息
    token_path = present_dir / "delegation_token.json"
    policy_json_str = ""
    if token_path.exists():
        policy_json_str = token_path.read_text()

    sessions[session_id]["verified"] = True

    return jsonify({
        "status": "ok",
        "verification": {
            "overall": overall,
            "checks": checks,
            "raw_output": stdout.strip(),
            "policy_json": policy_json_str,
        },
    })


@app.route("/api/reset", methods=["POST"])
def reset():
    data = request.get_json(silent=True) or {}
    session_id = data.get("session_id", "")
    if session_id in sessions:
        del sessions[session_id]
    return jsonify({"status": "ok"})


if __name__ == "__main__":
    SESSIONS_DIR.mkdir(parents=True, exist_ok=True)
    print(f"  Issuer binary   : {ISSUER_BIN}")
    print(f"  Alice binary    : {ALICE_BIN}")
    print(f"  Agent binary    : {AGENT_BIN}")
    print(f"  Verifier binary : {VERIFIER_BIN}")
    print(f"  Sessions dir    : {SESSIONS_DIR}")
    app.run(host="0.0.0.0", port=5001, debug=True)
