//
// Created by admin on 2025/11/5.
//
#include "myhook.h"
#include <vector>
#include <cstdint>
#include <android/log.h>
#include <map>
#include <string>
#include <sstream>
// ---------------------------------------------------------------
// 全局映射表：Tag → 名称
// ---------------------------------------------------------------
std::map<uint32_t, std::string> tagNames = {
        // 基本类型
        {0x01, "BOOLEAN"}, {0x02, "INTEGER"}, {0x03, "BIT STRING"}, {0x04, "OCTET STRING"},
        {0x05, "NULL"}, {0x06, "OBJECT IDENTIFIER"}, {0x07, "OBJECT DESCRIPTOR"}, {0x08, "EXTERNAL"},
        {0x09, "REAL"}, {0x0A, "ENUMERATED"}, {0x0B, "EMBEDDED PDV"}, {0x0C, "UTF8String"},
        {0x10, "SEQUENCE"}, {0x11, "SET"}, {0x12, "NumericString"}, {0x13, "PrintableString"},
        {0x14, "T61String"}, {0x15, "VideotexString"}, {0x16, "IA5String"}, {0x17, "UTCTime"},
        {0x18, "GeneralizedTime"}, {0x19, "GraphicString"}, {0x1A, "VisibleString"},
        {0x1B, "GeneralString"}, {0x1C, "UniversalString"}, {0x1E, "BMPString"},
        {0x20, "SEQUENCE OF"}, {0x21, "SET OF"}, {0x30, "SEQUENCE"}, {0x31, "SET"},

        // 上下文/应用标签
        {0x60, "[APPLICATION 0]"}, {0x61, "[APPLICATION 1]"},
        {0xA0, "[CONTEXT 0]"}, {0xA1, "[CONTEXT 1]"}, {0xA2, "[CONTEXT 2]"}, {0xA3, "[CONTEXT 3]"},
        {0xA4, "[CONTEXT 4]"}, {0xA5, "[CONTEXT 5]"}, {0xA6, "[CONTEXT 6]"}, {0xA7, "[CONTEXT 7]"},
        {0xA8, "[CONTEXT 8]"}, {0xA9, "[CONTEXT 9]"}, {0xAA, "[CONTEXT 10]"}, {0xAB, "[CONTEXT 11]"},
        {0xAC, "[CONTEXT 12]"}, {0xAD, "[CONTEXT 13]"}, {0xAE, "[CONTEXT 14]"}, {0xAF, "[CONTEXT 15]"},

        // PKCS#7 特有
        {0x80, "[PKCS7 CONTENT TYPE]"}, {0x81, "[PKCS7 DATA]"}, {0x82, "[PKCS7 SIGNED DATA]"},
        {0x83, "[PKCS7 CERTIFICATES]"}, {0x84, "[PKCS7 SIGNER INFO]"}, {0x85, "[PKCS7 CRL]"}
};

// ---------------------------------------------------------------
// 全局映射表：OID → 算法名称
// ---------------------------------------------------------------
std::map<std::string, std::string> algorithmOids = {
        {"1.2.840.113549.1.1.1", "RSAENCRYPTION"},
        {"1.2.840.113549.1.1.4", "MD5WITHRSA"}, {"1.2.840.113549.1.1.5", "SHA1WITHRSA"},
        {"1.2.840.113549.1.1.11", "SHA256WITHRSA"}, {"1.2.840.113549.1.1.12", "SHA384WITHRSA"},
        {"1.2.840.113549.1.1.13", "SHA512WITHRSA"},
        {"1.2.840.113549.2.5", "MD5"}, {"1.3.14.3.2.26", "SHA1"},
        {"2.16.840.1.101.3.4.2.1", "SHA256"}, {"2.16.840.1.101.3.4.2.2", "SHA384"},
        {"2.16.840.1.101.3.4.2.3", "SHA512"},
        {"1.2.840.113549.1.9.16.1.1", "PKCS7-DATA"}, {"1.2.840.113549.1.9.16.1.2", "PKCS7-SIGNED-DATA"},
        {"1.2.840.113549.1.9.16.1.4", "PKCS7-ENVELOPED-DATA"}, {"1.2.840.113549.1.9.16.1.7", "PKCS7-DIGESTED-DATA"},
        {"1.2.840.113549.1.9.16.1.8", "PKCS7-ENCRYPTED-DATA"},
        {"1.2.840.113549.1.9.16.1.27", "ANDROID-APK-SIGNING-BLOCK"},
        {"1.2.840.113549.1.9.16.1.28", "ANDROID-APK-V2-SIGNATURE"},
        {"1.2.840.113549.1.9.16.1.29", "ANDROID-APK-V3-SIGNATURE"},
        {"1.2.840.113549.1.9.16.1.30", "ANDROID-APK-V4-SIGNATURE"},
        {"1.2.840.15245.7", "CUSTOM-EXTENSION"},
        {"1.2.840.113549.1.7.1", "PKCS7-DATA"}, {"1.2.840.113549.1.7.2", "PKCS7-SIGNED-DATA"},
        {"1.2.840.113549.1.7.3", "PKCS7-ENVELOPED-DATA"}, {"1.2.840.113549.1.7.5", "PKCS7-DIGESTED-DATA"},
        {"1.2.840.113549.1.7.6", "PKCS7-ENCRYPTED-DATA"}
};

// ---------------------------------------------------------------
// TLV 结构体（普通指针）
// ---------------------------------------------------------------
struct TLV {
    uint32_t               T = 0;
    uint32_t               L = 0;
    std::vector<uint8_t>   V;
    TLV*                   next = nullptr;
};

// ---------------------------------------------------------------
// 解析 BER 长度
// ---------------------------------------------------------------
static size_t parseBERLength(const uint8_t* data, size_t len, size_t& offset, uint32_t& outLen)
{
    if (offset >= len) return offset;
    uint8_t first = data[offset++];
    if (first < 0x80) {
        outLen = first;
        return offset;
    }
    uint8_t byteCount = first & 0x7F;
    if (byteCount == 0 || byteCount > 4 || offset + byteCount > len) {
        LOGE("Invalid BER length at offset=%zu", offset - 1);
        return offset - 1;
    }
    outLen = 0;
    for (uint8_t i = 0; i < byteCount; ++i) {
        outLen = (outLen << 8) | data[offset++];
    }
    return offset;
}

// ---------------------------------------------------------------
// 解析单个 TLV
// ---------------------------------------------------------------
static size_t parseTLV(const uint8_t* data, size_t length, size_t offset, TLV& tlv)
{
    if (offset >= length) return offset;
    uint8_t first = data[offset++];
    if (first == 0) { tlv.T = 0; tlv.L = 0; return offset; }

    if ((first & 0x1F) == 0x1F) {
        tlv.T = first;
        while (offset < length) {
            uint8_t b = data[offset++];
            tlv.T = (tlv.T << 8) | b;
            if ((b & 0x80) == 0) break;
        }
    } else {
        tlv.T = first;
    }

    uint32_t lenVal = 0;
    size_t oldOffset = offset;
    offset = parseBERLength(data, length, offset, lenVal);
    if (offset == oldOffset) return oldOffset;
    tlv.L = lenVal;

    if (offset + tlv.L > length) {
        LOGE("Value overflow: need %u, remain %zu", tlv.L, length - offset);
        tlv.L = 0;
        return offset;
    }

    tlv.V.assign(data + offset, data + offset + tlv.L);
    offset += tlv.L;
    return offset;
}

// ---------------------------------------------------------------
// 判断是否为构造类型（支持嵌套）
// ---------------------------------------------------------------
static bool isContainerTag(uint32_t tag) {
    if (tag == 0x30 || tag == 0x31) return true;
    if ((tag & 0xC0) == 0x80 && (tag & 0x20)) return true;
    if ((tag & 0xE0) == 0xA0 && (tag & 0x20)) return true;
    return false;
}

// ---------------------------------------------------------------
// 解析 OID 为字符串
// ---------------------------------------------------------------
static std::string parseOID(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";

    std::ostringstream oss;
    uint64_t value = 0;
    bool first = true;

    for (size_t i = 0; i < data.size(); ++i) {
        uint8_t byte = data[i];
        value = (value << 7) | (byte & 0x7F);

        // 最高位为 0，表示本子节点结束
        if ((byte & 0x80) == 0) {
            if (first) {
                // 第一个值：X.Y = value / 40 . value % 40
                uint32_t x = value / 40;
                uint32_t y = value % 40;
                oss << x << "." << y;
                first = false;
            } else {
                oss << "." << value;
            }
            value = 0;
        }
    }
    return oss.str();
}

// ---------------------------------------------------------------
// 递归解析并打印（带映射表）
// ---------------------------------------------------------------
static void parseAndPrintNested(const std::vector<uint8_t>& data, int depth = 0)
{
    size_t offset = 0;
    TLV* head = nullptr;
    TLV* tail = nullptr;

    while (offset < data.size()) {
        TLV* tlv = new TLV;
        size_t newOffset = parseTLV(data.data(), data.size(), offset, *tlv);
        if (newOffset == offset || (tlv->T == 0 && tlv->L == 0)) {
            delete tlv;
            break;
        }
        offset = newOffset;
        if (!head) head = tail = tlv;
        else { tail->next = tlv; tail = tlv; }
    }

    TLV* cur = head;
    while (cur) {
        char indent[64] = {0};
        for (int i = 0; i < depth && i < 30; ++i) indent[i] = ' ';

        // Tag 名称
        std::string tagName = tagNames.count(cur->T) ? tagNames[cur->T] : "UNKNOWN";
        LOGI("%sTag: 0x%02X [%s], Len: %u", indent, cur->T, tagName.c_str(), cur->L);

        // OID 特殊处理
        if (cur->T == 0x06 && cur->L > 0) {
            std::string oid = parseOID(cur->V);
            std::string alg = algorithmOids.count(oid) ? algorithmOids[oid] : "UNKNOWN-ALG";
            LOGI("%s  └─OID: %s [%s]", indent, oid.c_str(), alg.c_str());
        }
        else if (cur->L > 0 && isContainerTag(cur->T)) {
            LOGI("%s  └─STRUCT (%u bytes)", indent, cur->L);
            parseAndPrintNested(cur->V, depth + 1);
        }
        else if (cur->L <= 64) {
            char line[200] = {0};
            char* p = line;
            for (size_t i = 0; i < cur->V.size(); ++i) {
                p += snprintf(p, 4, "%02X ", cur->V[i]);
            }
            LOGI("%s  └─Value: %s", indent, line);
        }
        else {
            LOGI("%s  └─Value: %u bytes (skipped)", indent, cur->L);
        }

        TLV* tmp = cur;
        cur = cur->next;
        delete tmp;
    }
}

// ---------------------------------------------------------------
// 入口
// ---------------------------------------------------------------
void v1SigParse(const uint8_t* data, size_t length)
{
    LOGI("=== Start parsing V1 signature (%zu bytes) ===", length);
    std::vector<uint8_t> buffer(data, data + length);
    parseAndPrintNested(buffer);
    LOGI("=== Parsing finished ===");
}

// ---------------------------------------------------------------
// JNI 接口
// ---------------------------------------------------------------
extern "C"
JNIEXPORT void JNICALL
Java_com_hookeasy_liuhookworld_SignatureParser_nativeParseV1Signature(
        JNIEnv* env, jclass, jbyteArray rsa_bytes)
{
    if (!rsa_bytes) { LOGE("rsa_bytes is null"); return; }
    jsize length = env->GetArrayLength(rsa_bytes);
    if (length <= 0) { LOGE("rsa_bytes length <= 0"); return; }

    std::vector<uint8_t> rsaData(length);
    env->GetByteArrayRegion(rsa_bytes, 0, length, reinterpret_cast<jbyte*>(rsaData.data()));
    if (env->ExceptionCheck()) { LOGE("GetByteArrayRegion failed"); env->ExceptionClear(); return; }

    v1SigParse(rsaData.data(), rsaData.size());
}