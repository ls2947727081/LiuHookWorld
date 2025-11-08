//
// Created by admin on 2025/11/5.
//
#include "myhook.h"
#include <vector>
#include <cstdint>
#include "iostream"

class TLV{
public:
    int T;
    int L;
    std::vector<uint8_t> V;                  // Value
    TLV* next  = nullptr;                 // 下一个 TLV（同层）
};

size_t parseTLV(const uint8_t* data, size_t length, size_t offset, TLV& tlv){
    if (offset>=length){
        return offset;
    }
    int16_t T = data[offset++];
    if (T == 0){

    }
    return 0;
}

void v1SigParse(const uint8_t* data, size_t length) {
    // 在这里解析 RSA 文件的字节数据
    size_t offset = 0;
    while (offset<length){
        TLV tlv;
        offset = parseTLV(data, length, offset, tlv);
        std::cout << "Tag: 0x" << std::hex << tlv.T << ", Len: " << std::dec << tlv.L << std::endl;
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_hookeasy_liuhookworld_SignatureParser_nativeParseV1Signature(
        JNIEnv *env, jclass clazz, jbyteArray rsa_bytes) {

    // 1️⃣ 获取字节数组长度
    jsize length = env->GetArrayLength(rsa_bytes);
    if (length <= 0) return;

    // 2️⃣ 申请缓冲区并获取数组内容
    jbyte *bytes = env->GetByteArrayElements(rsa_bytes, nullptr);

    // 3️⃣ 拷贝到 C++ 容器中（方便后续解析）
    std::vector<uint8_t> rsaData(length);
    for (jsize i = 0; i < length; i++) {
        rsaData[i] = static_cast<uint8_t>(bytes[i]);
    }

    // 🔹 此处调用你的解析逻辑（比如 v1SigParse(rsaData)）
    v1SigParse(rsaData.data(), rsaData.size());

    // 4️⃣ 释放 Java 数组引用
    env->ReleaseByteArrayElements(rsa_bytes, bytes, JNI_ABORT);
}