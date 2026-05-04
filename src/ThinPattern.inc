// Generated from source/Thin/Thinsub.cpp pattern helpers; adapted to local PixelF-compatible types.
//*****************************************************************************
template<typename PixelType>
inline bool CompPx(typename PixelTraits<PixelType>::InfoType* ti, PixelType p)
{
    bool ret = false;
    ThinPixel8 p8 = PixelTraits<PixelType>::ConvertTo8bit(p);

    for (int i = 0; i < ti->target_color_count; i++) {
        if ((std::abs((int)ti->color8[i].red - (int)p8.red) <= ti->level)
            && (std::abs((int)ti->color8[i].green - (int)p8.green) <= ti->level)
            && (std::abs((int)ti->color8[i].blue - (int)p8.blue) <= ti->level)) {
            ret = true;
            break;
        }
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline bool CompPxs(typename PixelTraits<PixelType>::InfoType* ti,
    PixelType p1, PixelType p2, PixelType p3)
{
    bool ret = false;
    ThinPixel8 pp1 = PixelTraits<PixelType>::ConvertTo8bit(p1);
    ThinPixel8 pp2 = PixelTraits<PixelType>::ConvertTo8bit(p2);
    ThinPixel8 pp3 = PixelTraits<PixelType>::ConvertTo8bit(p3);

    if ((std::abs((int)pp1.red - (int)pp2.red) <= ti->level)
        && (std::abs((int)pp1.green - (int)pp2.green) <= ti->level)
        && (std::abs((int)pp1.blue - (int)pp2.blue) <= ti->level)) {
        if ((std::abs((int)pp1.red - (int)pp3.red) <= ti->level)
            && (std::abs((int)pp1.green - (int)pp3.green) <= ti->level)
            && (std::abs((int)pp1.blue - (int)pp3.blue) <= ti->level)) {
            ret = true;
        }
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline bool CompPxs2(typename PixelTraits<PixelType>::InfoType* ti,
    PixelType p1, PixelType p2)
{
    bool ret = false;
    ThinPixel8 pp1 = PixelTraits<PixelType>::ConvertTo8bit(p1);
    ThinPixel8 pp2 = PixelTraits<PixelType>::ConvertTo8bit(p2);

    if ((std::abs((int)pp1.red - (int)pp2.red) <= ti->level)
        && (std::abs((int)pp1.green - (int)pp2.green) <= ti->level)
        && (std::abs((int)pp1.blue - (int)pp2.blue) <= ti->level)) {
        ret = true;
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline void scanlineCopy(typename PixelTraits<PixelType>::InfoType* ti, int y)
{
    int v = 0;
    if (y == 0) {
        for (int i = 0; i < ti->w; i++) {
            ti->scanline[i] = ti->data[i];
            ti->scanline[i + ti->wt] = ti->data[i];
            ti->scanline[i + ti->wt2] = ti->data[i + ti->wt];
        }
    }
    else {
        for (int i = 0; i < ti->w; i++) {
            ti->scanline[i] = ti->scanline[i + ti->wt];
            ti->scanline[i + ti->wt] = ti->scanline[i + ti->wt2];
        }
        if (y < ti->h - 1) {
            v = (y + 1) * ti->wt;
            for (int i = 0; i < ti->w; i++) {
                ti->scanline[i + ti->wt2] = ti->data[i + v];
            }
        }
    }
}

//*****************************************************************************
// �}���`�X���b�h�Ή�: �X���b�h���[�J����scanlineCopy
//*****************************************************************************
template<typename PixelType>
inline void scanlineCopyLocal(typename PixelTraits<PixelType>::InfoType* ti,
    PixelType* localScanline, int y)
{
    int v = 0;
    if (y == 0) {
        for (int i = 0; i < ti->w; i++) {
            localScanline[i] = ti->data[i];
            localScanline[i + ti->wt] = ti->data[i];
            localScanline[i + ti->wt2] = ti->data[i + ti->wt];
        }
    }
    else {
        for (int i = 0; i < ti->w; i++) {
            localScanline[i] = localScanline[i + ti->wt];
            localScanline[i + ti->wt] = localScanline[i + ti->wt2];
        }
        if (y < ti->h - 1) {
            v = (y + 1) * ti->wt;
            for (int i = 0; i < ti->w; i++) {
                localScanline[i + ti->wt2] = ti->data[i + v];
            }
        }
    }
}

//*****************************************************************************
template<typename PixelType>
inline PixelType getScanLine(typename PixelTraits<PixelType>::InfoType* ti,
    int offsetX, int offsetY)
{
    int xx = ti->nowX + offsetX;
    int yy = 1 + offsetY;
    if (xx < 0) {
        xx = 0;
    }
    else if (xx >= ti->w) {
        xx = ti->w - 1;
    }
    if (yy < 0) {
        yy = 0;
    }
    else if (yy > 2) {
        yy = 2;
    }
    return ti->scanline[xx + yy * ti->wt];
}

//*****************************************************************************
// �}���`�X���b�h�Ή�: �X���b�h���[�J����getScanLine�inowX, nowY���p�����[�^�Ƃ��Ď󂯎��j
//*****************************************************************************
template<typename PixelType>
inline PixelType getScanLineLocal(typename PixelTraits<PixelType>::InfoType* ti,
    PixelType* localScanline, int nowX, int offsetX, int offsetY)
{
    int xx = nowX + offsetX;
    int yy = 1 + offsetY;
    if (xx < 0) {
        xx = 0;
    }
    else if (xx >= ti->w) {
        xx = ti->w - 1;
    }
    if (yy < 0) {
        yy = 0;
    }
    else if (yy > 2) {
        yy = 2;
    }
    return localScanline[xx + yy * ti->wt];
}

//*****************************************************************************
template<typename PixelType>
inline bool getData(typename PixelTraits<PixelType>::InfoType* ti,
    int offsetX, int offsetY)
{
    int xx = ti->nowX + offsetX;
    int yy = ti->nowY + offsetY;
    if (xx < 0) {
        xx = 0;
    }
    else if (xx >= ti->w) {
        xx = ti->w - 1;
    }
    if (yy < 0) {
        yy = 0;
    }
    else if (yy >= ti->h) {
        yy = ti->h - 1;
    }
    PixelType p = ti->data[xx + yy * ti->wt];
    return CompPx<PixelType>(ti, p);
}

//*****************************************************************************
// �}���`�X���b�h�Ή�: getDataLocal�inowX, nowY���p�����[�^�Ƃ��Ď󂯎��j
//*****************************************************************************
template<typename PixelType>
inline bool getDataLocal(typename PixelTraits<PixelType>::InfoType* ti,
    int nowX, int nowY, int offsetX, int offsetY)
{
    int xx = nowX + offsetX;
    int yy = nowY + offsetY;
    if (xx < 0) {
        xx = 0;
    }
    else if (xx >= ti->w) {
        xx = ti->w - 1;
    }
    if (yy < 0) {
        yy = 0;
    }
    else if (yy >= ti->h) {
        yy = ti->h - 1;
    }
    PixelType p = ti->data[xx + yy * ti->wt];
    return CompPx<PixelType>(ti, p);
}

//*****************************************************************************
template<typename PixelType>
inline PixelType minColor(PixelType s, PixelType d)
{
    ThinPixel8 s8 = PixelTraits<PixelType>::ConvertTo8bit(s);
    ThinPixel8 d8 = PixelTraits<PixelType>::ConvertTo8bit(d);

    int ss = (int)s8.red + (int)s8.green + (int)s8.blue;
    int dd = (int)d8.red + (int)d8.green + (int)d8.blue;

    if (ss < dd) {
        return s;
    }
    else {
        return d;
    }
}

//*****************************************************************************
// �s�N�Z���X�L�b�v����i�������p�j
//*****************************************************************************
template<typename PixelType>
inline bool shouldSkipPixel(typename PixelTraits<PixelType>::InfoType* ti, const PixelType& color)
{
    if (!ti->white && !ti->alphaZero) return false;

    ThinPixel8 c8 = PixelTraits<PixelType>::ConvertTo8bit(color);

    if (ti->white) {
        if ((c8.blue == 255) && (c8.green == 255)
            && (c8.red == 255) && (c8.alpha == 255)) {
            return true;
        }
    }
    if (ti->alphaZero) {
        if (c8.alpha == 0) return true;
    }
    return false;
}

//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPatEdge(typename PixelTraits<PixelType>::InfoType* ti)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;

    PixelType table[8];
    table[0] = getScanLine<PixelType>(ti, -1, -1);
    table[1] = getScanLine<PixelType>(ti, 0, -1);
    table[2] = getScanLine<PixelType>(ti, 1, -1);
    table[3] = getScanLine<PixelType>(ti, -1, 0);
    table[4] = getScanLine<PixelType>(ti, 1, 0);
    table[5] = getScanLine<PixelType>(ti, -1, 1);
    table[6] = getScanLine<PixelType>(ti, 0, 1);
    table[7] = getScanLine<PixelType>(ti, 1, 1);

    int pat = 0;
    int patCount = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
            patCount++;
        }
    }

    if ((pat == 0x8F) || (pat == 0x8E)) {
        if (CompPxs<PixelType>(ti, table[1], table[2], table[3]) == true) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
        return ret;
    }
    else if ((pat == 0x6D) || (pat == 0x4D)) {
        if (CompPxs<PixelType>(ti, table[0], table[3], table[6]) == true) {
            ret.drawFlag = true;
            ret.color = table[0];
        }
        return ret;
    }
    else if ((pat == 0xF1) || (pat == 0x71)) {
        if (CompPxs<PixelType>(ti, table[4], table[5], table[6]) == true) {
            ret.drawFlag = true;
            ret.color = table[4];
        }
        return ret;
    }
    else if ((pat == 0x86) || (pat == 0xB2)) {
        if (CompPxs<PixelType>(ti, table[1], table[4], table[7]) == true) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
        return ret;
    }
    else if ((pat == 0x35) || (pat == 0x33)) {
        if (CompPxs<PixelType>(ti, table[0], table[1], table[4]) == true) {
            ret.drawFlag = true;
            ret.color = table[0];
        }
        return ret;
    }
    else if ((pat == 0xD5) || (pat == 0x55)) {
        if (CompPxs<PixelType>(ti, table[2], table[4], table[6]) == true) {
            ret.drawFlag = true;
            ret.color = table[2];
        }
        return ret;
    }
    else if ((pat == 0xEC) || (pat == 0xCC)) {
        if (CompPxs<PixelType>(ti, table[3], table[6], table[7]) == true) {
            ret.drawFlag = true;
            ret.color = table[3];
        }
        return ret;
    }
    else if ((pat == 0xAB) || (pat == 0xAA)) {
        if (CompPxs<PixelType>(ti, table[1], table[3], table[5]) == true) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
        return ret;
    }
    else if ((pat == 0xED) || (pat == 0xCD)) {
        if (CompPxs2<PixelType>(ti, table[3], table[6]) == true) {
            ret.drawFlag = true;
            ret.color = table[3];
        }
        return ret;
    }
    else if ((pat == 0xAF) || (pat == 0xAE)) {
        if (CompPxs2<PixelType>(ti, table[1], table[3]) == true) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
        return ret;
    }
    else if ((pat == 0xB7) || (pat == 0xB3)) {
        if (CompPxs2<PixelType>(ti, table[1], table[4]) == true) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
        return ret;
    }
    else if ((pat == 0xF5) || (pat == 0x75)) {
        if (CompPxs2<PixelType>(ti, table[4], table[6]) == true) {
            ret.drawFlag = true;
            ret.color = table[4];
        }
    }
    return ret;
}

//*****************************************************************************
// �}���`�X���b�h�Ή�: getPatEdgeLocal
//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPatEdgeLocal(
    typename PixelTraits<PixelType>::InfoType* ti, PixelType* localScanline, int nowX)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;

    PixelType table[8];
    table[0] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, -1);
    table[1] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 0, -1);
    table[2] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, -1);
    table[3] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, 0);
    table[4] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, 0);
    table[5] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, 1);
    table[6] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 0, 1);
    table[7] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, 1);

    int pat = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
        }
    }

    if ((pat == 0x8F) || (pat == 0x8E)) {
        if (CompPxs<PixelType>(ti, table[1], table[2], table[3])) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
    }
    else if ((pat == 0x6D) || (pat == 0x4D)) {
        if (CompPxs<PixelType>(ti, table[0], table[3], table[6])) {
            ret.drawFlag = true;
            ret.color = table[0];
        }
    }
    else if ((pat == 0xF1) || (pat == 0x71)) {
        if (CompPxs<PixelType>(ti, table[4], table[5], table[6])) {
            ret.drawFlag = true;
            ret.color = table[4];
        }
    }
    else if ((pat == 0x86) || (pat == 0xB2)) {
        if (CompPxs<PixelType>(ti, table[1], table[4], table[7])) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
    }
    else if ((pat == 0x35) || (pat == 0x33)) {
        if (CompPxs<PixelType>(ti, table[0], table[1], table[4])) {
            ret.drawFlag = true;
            ret.color = table[0];
        }
    }
    else if ((pat == 0xD5) || (pat == 0x55)) {
        if (CompPxs<PixelType>(ti, table[2], table[4], table[6])) {
            ret.drawFlag = true;
            ret.color = table[2];
        }
    }
    else if ((pat == 0xEC) || (pat == 0xCC)) {
        if (CompPxs<PixelType>(ti, table[3], table[6], table[7])) {
            ret.drawFlag = true;
            ret.color = table[3];
        }
    }
    else if ((pat == 0xAB) || (pat == 0xAA)) {
        if (CompPxs<PixelType>(ti, table[1], table[3], table[5])) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
    }
    else if ((pat == 0xED) || (pat == 0xCD)) {
        if (CompPxs2<PixelType>(ti, table[3], table[6])) {
            ret.drawFlag = true;
            ret.color = table[3];
        }
    }
    else if ((pat == 0xAF) || (pat == 0xAE)) {
        if (CompPxs2<PixelType>(ti, table[1], table[3])) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
    }
    else if ((pat == 0xB7) || (pat == 0xB3)) {
        if (CompPxs2<PixelType>(ti, table[1], table[4])) {
            ret.drawFlag = true;
            ret.color = table[1];
        }
    }
    else if ((pat == 0xF5) || (pat == 0x75)) {
        if (CompPxs2<PixelType>(ti, table[4], table[6])) {
            ret.drawFlag = true;
            ret.color = table[4];
        }
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPatDot(typename PixelTraits<PixelType>::InfoType* ti)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;

    PixelType table[8];
    table[0] = getScanLine<PixelType>(ti, -1, -1);
    table[1] = getScanLine<PixelType>(ti, 0, -1);
    table[2] = getScanLine<PixelType>(ti, 1, -1);
    table[3] = getScanLine<PixelType>(ti, -1, 0);
    table[4] = getScanLine<PixelType>(ti, 1, 0);
    table[5] = getScanLine<PixelType>(ti, -1, 1);
    table[6] = getScanLine<PixelType>(ti, 0, 1);
    table[7] = getScanLine<PixelType>(ti, 1, 1);

    int pat = 0;
    int patCount = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
            patCount++;
        }
    }

    if (pat == 0XE0) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[3], minColor<PixelType>(table[4], table[6]));
        return ret;
    }
    else if (pat == 0X29) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[3], table[6]));
        return ret;
    }
    else if (pat == 0X07) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[3], table[4]));
        return ret;
    }
    else if (pat == 0X94) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[4], table[6]));
        return ret;
    }
    return ret;
}

//*****************************************************************************
// �}���`�X���b�h�Ή�: getPatDotLocal
//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPatDotLocal(
    typename PixelTraits<PixelType>::InfoType* ti, PixelType* localScanline, int nowX)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;

    PixelType table[8];
    table[0] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, -1);
    table[1] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 0, -1);
    table[2] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, -1);
    table[3] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, 0);
    table[4] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, 0);
    table[5] = getScanLineLocal<PixelType>(ti, localScanline, nowX, -1, 1);
    table[6] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 0, 1);
    table[7] = getScanLineLocal<PixelType>(ti, localScanline, nowX, 1, 1);

    int pat = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
        }
    }

    if (pat == 0XE0) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[3], minColor<PixelType>(table[4], table[6]));
        return ret;
    }
    else if (pat == 0X29) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[3], table[6]));
        return ret;
    }
    else if (pat == 0X07) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[3], table[4]));
        return ret;
    }
    else if (pat == 0X94) {
        ret.drawFlag = true;
        ret.color = minColor<PixelType>(table[1], minColor<PixelType>(table[4], table[6]));
        return ret;
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPatDot2nd(typename PixelTraits<PixelType>::InfoType* ti)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;
    ret.dir = -1;

    PixelType table[8];
    table[0] = getScanLine<PixelType>(ti, -1, -1);
    table[1] = getScanLine<PixelType>(ti, 0, -1);
    table[2] = getScanLine<PixelType>(ti, 1, -1);
    table[3] = getScanLine<PixelType>(ti, -1, 0);
    table[4] = getScanLine<PixelType>(ti, 1, 0);
    table[5] = getScanLine<PixelType>(ti, -1, 1);
    table[6] = getScanLine<PixelType>(ti, 0, 1);
    table[7] = getScanLine<PixelType>(ti, 1, 1);

    int pat = 0;
    int patCount = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
            patCount++;
        }
    }

    if ((pat & 0x1F) == 0X1D) {
        ret.drawFlag = true;
        ret.color = table[3];
        ret.dir = 2;
        return ret;
    }
    else if ((pat & 0xD6) == 0XC6) {
        ret.drawFlag = true;
        ret.color = table[0];
        ret.dir = 3;
        return ret;
    }
    else if ((pat & 0xF8) == 0XB8) {
        ret.drawFlag = true;
        ret.color = table[0];
        ret.dir = 0;
        return ret;
    }
    else if ((pat & 0x6B) == 0X63) {
        ret.drawFlag = true;
        ret.color = table[1];
        ret.dir = 1;
        return ret;
    }
    return ret;
}

//*****************************************************************************
template<typename PixelType>
inline typename PixelTraits<PixelType>::BakType getPat(typename PixelTraits<PixelType>::InfoType* ti)
{
    typedef typename PixelTraits<PixelType>::BakType ThinBak;
    ThinBak ret;
    ret.drawFlag = false;

    PixelType table[8];
    table[0] = getScanLine<PixelType>(ti, -1, -1);
    table[1] = getScanLine<PixelType>(ti, 0, -1);
    table[2] = getScanLine<PixelType>(ti, 1, -1);
    table[3] = getScanLine<PixelType>(ti, -1, 0);
    table[4] = getScanLine<PixelType>(ti, 1, 0);
    table[5] = getScanLine<PixelType>(ti, -1, 1);
    table[6] = getScanLine<PixelType>(ti, 0, 1);
    table[7] = getScanLine<PixelType>(ti, 1, 1);

    int pat = 0;
    int patCount = 0;
    for (int i = 0; i < 8; i++) {
        pat <<= 1;
        if (CompPx<PixelType>(ti, table[i])) {
            pat = pat | 1;
            patCount++;
        }
    }

    if (patCount > 7) {
        return ret;
    }
    else if (patCount <= 0) {
        ret.drawFlag = true;
        PixelType c = minColor<PixelType>(table[1], table[3]);
        c = minColor<PixelType>(c, table[4]);
        c = minColor<PixelType>(c, table[6]);
        ret.color = c;
        return ret;
    }
    else if (patCount == 7) {
        if (pat == 0xBF) {
            if (getData<PixelType>(ti, 0, 1) == true) {
                ret.drawFlag = true;
                ret.color = table[1];
            }
            return ret;
        }
        if (pat == 0xEF) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[3];
            }
            return ret;
        }
        if (pat == 0xFD) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[6];
            }
            return ret;
        }
        if (pat == 0x7F) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[4];
            }
            return ret;
        }
    }
    else if (patCount == 6) {
        if ((pat == 0x9F) || (pat == 0x3F)) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[1];
            }
            return ret;
        }
        else if ((pat == 0x6F) || (pat == 0xEB)) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[3];
            }
            return ret;
        }
        else if ((pat == 0xF9) || (pat == 0xFC)) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[6];
            }
            return ret;
        }
        else if ((pat == 0xF6) || (pat == 0xD7)) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[4];
            }
            return ret;
        }
    }
    else if (patCount == 5) {
        if (pat == 0xF8) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[6];
            }
            return ret;
        }
        if (pat == 0x1F) {
            if (getData<PixelType>(ti, 0, 1) == true) {
                ret.drawFlag = true;
                ret.color = table[1];
            }
            return ret;
        }
        else if (pat == 0x6B) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[3];
            }
            return ret;
        }
        else if (pat == 0xF8) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[4];
            }
            return ret;
        }
        else if (pat == 0x97) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[1], table[4]);
            return ret;
        }
        else if (pat == 0x2F) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[1], table[3]);
            return ret;
        }
        else if (pat == 0xE9) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[3], table[6]);
            return ret;
        }
        else if (pat == 0xF4) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[4], table[6]);
            return ret;
        }
        else if (pat == 0x5D) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[6];
            }
            return ret;
        }
        else if (pat == 0x73) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[4];
            }
            return ret;
        }
        else if (pat == 0xBA) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = table[1];
            }
            return ret;
        }
        else if (pat == 0xCE) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = table[3];
            }
            return ret;
        }
    }
    else if (patCount == 4) {
        if ((pat == 0x0F) || (pat == 0x2B)) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[1], table[3]);
            return ret;
        }
        else if ((pat == 0x69) || (pat == 0xE8)) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[3], table[6]);
            return ret;
        }
        else if ((pat == 0xF0) || (pat == 0xD4)) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[4], table[6]);
            return ret;
        }
        else if ((pat == 0x96) || (pat == 0x17)) {
            ret.drawFlag = true;
            ret.color = minColor<PixelType>(table[1], table[4]);
            return ret;
        }
        else if (pat == 0xC9) {
            if ((getData<PixelType>(ti, 0, -1) == true) && (getData<PixelType>(ti, 1, 0) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[3], table[6]);
            }
            return ret;
        }
        else if (pat == 0x74) {
            if ((getData<PixelType>(ti, 0, -1) == true) && (getData<PixelType>(ti, -1, 0) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[4], table[6]);
            }
            return ret;
        }
        else if (pat == 0x93) {
            if ((getData<PixelType>(ti, -1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[4]);
            }
            return ret;
        }
        else if (pat == 0x2E) {
            if ((getData<PixelType>(ti, 1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[3]);
            }
            return ret;
        }
    }
    else if (patCount == 3) {
        if (pat == 0xE0) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(minColor<PixelType>(table[3], table[4]), table[6]);
            }
            return ret;
        }
        else if (pat == 0x94) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(minColor<PixelType>(table[1], table[4]), table[6]);
            }
            return ret;
        }
        else if (pat == 0x07) {
            if (getData<PixelType>(ti, 0, 1) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(minColor<PixelType>(table[1], table[3]), table[4]);
            }
            return ret;
        }
        else if (pat == 0x29) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(minColor<PixelType>(table[1], table[3]), table[6]);
            }
            return ret;
        }
        else if (pat == 0x70) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[4], table[6]);
            }
            return ret;
        }
        else if (pat == 0x13) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[4], table[1]);
            }
            return ret;
        }
        else if (pat == 0x2A) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[3]);
            }
            return ret;
        }
        else if (pat == 0x92) {
            if (getData<PixelType>(ti, -1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[4]);
            }
            return ret;
        }
        else if (pat == 0x2A) {
            if (getData<PixelType>(ti, 0, -1) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[3], table[6]);
            }
            return ret;
        }
        else if (pat == 0x0E) {
            if (getData<PixelType>(ti, 1, 0) == true) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[3]);
            }
            return ret;
        }
        if (pat == 0xD0) {
            if ((getData<PixelType>(ti, 0, -1) == true) && (getData<PixelType>(ti, -1, 0) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[4], table[6]);
            }
            return ret;
        }
        if (pat == 0x16) {
            if ((getData<PixelType>(ti, -1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[4]);
            }
            return ret;
        }
        if (pat == 0x0B) {
            if ((getData<PixelType>(ti, 1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[3]);
            }
            return ret;
        }
    }
    else if (patCount == 2) {
        if (pat == 0x50) {
            if ((getData<PixelType>(ti, 0, -1) == true) && (getData<PixelType>(ti, -1, 0) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[4], table[6]);
            }
            return ret;
        }
        else if (pat == 0x12) {
            if ((getData<PixelType>(ti, -1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[4]);
            }
            return ret;
        }
        else if (pat == 0x0A) {
            if ((getData<PixelType>(ti, 1, 0) == true) && (getData<PixelType>(ti, 0, 1) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[1], table[3]);
            }
            return ret;
        }
        else if (pat == 0x48) {
            if ((getData<PixelType>(ti, 0, -1) == true) && (getData<PixelType>(ti, 1, 0) == true)) {
                ret.drawFlag = true;
                ret.color = minColor<PixelType>(table[3], table[6]);
            }
            return ret;
        }
    }
    return ret;
}
