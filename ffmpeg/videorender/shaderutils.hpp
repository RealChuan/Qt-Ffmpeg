// Most of this code comes from mpv

#ifndef SHADERUTILS_HPP
#define SHADERUTILS_HPP

#include <QGenericMatrix>
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

void passGama(QByteArray &frag, AVColorTransferCharacteristic colortTrc);

void passDeGama(QByteArray &frag, AVColorTransferCharacteristic srcColortTrc);

void passOotf(QByteArray &frag, float peak, AVColorTransferCharacteristic colortTrc);

void passInverseOotf(QByteArray &frag, float peak, AVColorTransferCharacteristic colortTrc);

auto convertPrimaries(QByteArray &header,
                      QByteArray &frag,
                      AVColorPrimaries srcPrimaries,
                      AVColorPrimaries dstPrimaries,
                      QMatrix3x3 &matrix) -> bool;

void finishFragment(QByteArray &frag);

void printShader(const QByteArray &frag);

} // namespace Ffmpeg::ShaderUtils

#endif // SHADERUTILS_HPP
