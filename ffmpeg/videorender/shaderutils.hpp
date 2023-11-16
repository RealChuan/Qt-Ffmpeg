#ifndef SHADERUTILS_HPP
#define SHADERUTILS_HPP

#include <QtCore>

extern "C" {
#include <libavutil/pixfmt.h>
}

#define GLSL(shader) #shader

#define MP_REF_WHITE 203.0
#define MP_REF_WHITE_HLG 3.17955

struct AVFrame;

namespace Ffmpeg::ShaderUtils {

auto trcNomPeak(AVColorTransferCharacteristic colortTrc) -> float;

auto trcIsHdr(AVColorTransferCharacteristic colortTrc) -> bool;

auto header() -> QByteArray;

auto beginFragment(QByteArray &frag, int format) -> bool;

void passLinearize(QByteArray &frag, AVColorTransferCharacteristic colortTrc);

void passDeLinearize(QByteArray &frag, AVColorTransferCharacteristic colortTrc);

void toneMap(QByteArray &frag, AVFrame *avFrame);

void finishFragment(QByteArray &frag);

void printShader(const QByteArray &frag);

} // namespace Ffmpeg::ShaderUtils

#endif // SHADERUTILS_HPP