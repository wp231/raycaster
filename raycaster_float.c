// 浮點數實現，用於測試/比較
//
// 這個圖表顯示了在 RayCasterFloat::Distance() 函數中使用的座標系統。
//
//              ^ rayA/
//     sin-     |    /   sin+
//     cos+     |   /    cos+
//     tan-     |  /     tan+
//              | /
//              |/
// ---------------------------->
//              |
//     sin-     |        sin+
//     cos-     |        cos-
//     tan+     |        tan-
//              |

#include "raycaster_float.h"
#include <math.h>
#include <stdlib.h>

#define P2P_DISTANCE(x1, y1, x2, y2)              \
    sqrt((float) (((x1) - (x2)) * ((x1) - (x2)) + \
                  ((y1) - (y2)) * ((y1) - (y2))))

// 定義結構，表示浮點數光線追踪器的狀態
typedef struct {
    float playerX;
    float playerY;
    float playerA;
} RayCasterFloat;


// 內部函數：RayCasterFloatTrace
// 參數：rayCaster - 光線追踪器
//       screenX - 螢幕上的 x 座標
//       screenY - 螢幕上的 y 座標（輸出）
//       textureNo - 貼圖編號（輸出）
//       textureX - 貼圖中的 x 座標（輸出）
//       textureY - 貼圖中的 y 座標（輸出）
//       textureStep - 貼圖步進（輸出）
// 說明：跟蹤光線，計算並輸出螢幕上的相應數據
static void RayCasterFloatTrace(RayCaster *rayCaster,
                                uint16_t screenX,
                                uint8_t *screenY,
                                uint8_t *textureNo,
                                uint8_t *textureX,
                                uint16_t *textureY,
                                uint16_t *textureStep);
// 內部函數：RayCasterFloatStart
// 參數：rayCaster - 光線追踪器
//       playerX - 玩家在 x 軸上的位置
//       playerY - 玩家在 y 軸上的位置
//       playerA - 玩家的角度
// 說明：初始化浮點數光線追踪器的起始狀態
static void RayCasterFloatStart(RayCaster *rayCaster,
                                uint16_t playerX,
                                uint16_t playerY,
                                int16_t playerA);
// 內部函數：RayCasterFloatDestruct
// 參數：rayCaster - 光線追踪器
// 說明：釋放浮點數光線追踪器的資源
static void RayCasterFloatDestruct(RayCaster *rayCaster);

// 函數：RayCasterFloatConstruct
// 返回：RayCaster* - 光線追踪器結構指針
// 說明：創建浮點光線追踪器實例
RayCaster *RayCasterFloatConstruct(void)
{
    // 創建基本光線追踪器實例
    RayCaster *rayCaster = RayCasterConstruct();
    RayCasterFloat *rayCasterFloat = malloc(sizeof(RayCasterFloat));

    // 檢查內存分配是否成功
    if (!rayCasterFloat) {
        rayCaster->Destruct(rayCaster);
        return NULL;
    }
    rayCaster->derived = rayCasterFloat;

    // 設置函數指針
    rayCaster->Start = RayCasterFloatStart;
    rayCaster->Trace = RayCasterFloatTrace;
    rayCaster->Destruct = RayCasterFloatDestruct;

    return rayCaster;
}

// 內部函數：RayCasterFloatIsWall
// 參數：rayX - 光線的 x 座標
//       rayY - 光線的 y 座標
// 返回：如果光線碰到牆壁，返回 true；否則返回 false
// 說明：檢查光線是否碰到牆壁
static bool RayCasterFloatIsWall(float rayX, float rayY)
{
    // 將浮點坐標轉換為地圖格子坐標
    float mapX = 0;
    float mapY = 0;
    modff(rayX, &mapX);
    modff(rayY, &mapY);
    int tileX = (int) mapX;
    int tileY = (int) mapY;

    // 檢查是否超出地圖邊界
    if (tileX < 0 || tileY < 0 || tileX >= MAP_X - 1 || tileY >= MAP_Y - 1) {
        return true;
    }

    // 檢查格子中的位是否為牆壁
    return g_map[(tileX >> 3) + (tileY << (MAP_XS - 3))] &
           (1 << (8 - (tileX & 0x7)));
}

// 函數：RayCasterFloatDistance
// 參數：playerX - 玩家的X座標
//       playerY - 玩家的Y座標
//       rayA - 光線的角度
//       hitOffset - 命中點的偏移量
//       hitDirection - 命中的方向（垂直或水平）
// 返回：命中點到玩家的距離
// 說明：計算光線與牆壁的交點，並返回命中點到玩家的距離
static float RayCasterFloatDistance(float playerX,
                                    float playerY,
                                    float rayA,
                                    float *hitOffset,
                                    int *hitDirection)
{
    // 角度正規化，確保在0到2π之間
    while (rayA < 0) {
        rayA += 2.0f * M_PI;
    }
    while (rayA >= 2.0f * M_PI) {
        rayA -= 2.0f * M_PI;
    }

    // 計算光線的斜率和垂直斜率
    // 計算tan和cot值
    float tanA = tan(rayA);
    float cotA = 1 / tanA;
    float rayX, rayY, vx, vy;
    float xOffset, yOffset, vertHitDis, horiHitDis;
    int depth = 0;
    // 射線尋找牆壁的最大嘗試次數
    const int maxDepth = 100;

    // 檢查垂直方向的射線
    depth = 0;
    vertHitDis = 0;
    if (sin(rayA) > 0.001) {
        // 射線向右
        rayX = (int) playerX + 1;
        rayY = (rayX - playerX) * cotA + playerY;
        xOffset = 1;
        yOffset = xOffset * cotA;
    } else if (sin(rayA) < -0.001) {
        // 射線向左
        rayX = (int) playerX - 0.001;
        rayY = (rayX - playerX) * cotA + playerY;
        xOffset = -1;
        yOffset = xOffset * cotA;
    } else {
        // 射線垂直
        rayX = playerX;
        rayY = playerY;
        xOffset = 0;
        yOffset = 0;
        depth = maxDepth;
    }

    // 尋找垂直方向的牆壁
    while (depth < maxDepth) {
        if (RayCasterFloatIsWall(rayX, rayY)) {
            vertHitDis = P2P_DISTANCE(playerX, playerY, rayX, rayY);
            break;
        } else {
            rayX += xOffset;
            rayY += yOffset;
            depth += 1;
        }
    }
    vx = rayX;
    vy = rayY;

    // 檢查水平方向的命中
    depth = 0;
    horiHitDis = 0;
    if (cos(rayA) > 0.001) {
        // 射線向上
        rayY = (int) playerY + 1;
        rayX = (rayY - playerY) * tanA + playerX;
        yOffset = 1;
        xOffset = yOffset * tanA;
    } else if (cos(rayA) < -0.001) {
        // 射線向下
        rayY = (int) playerY - 0.001;
        rayX = (rayY - playerY) * tanA + playerX;
        yOffset = -1;
        xOffset = yOffset * tanA;
    } else {
        // 射線水平
        rayX = playerX;
        rayY = playerY;
        xOffset = 0;
        yOffset = 0;
        depth = maxDepth;
    }

    // 尋找水平方向的牆壁
    while (depth < maxDepth) {
        if (RayCasterFloatIsWall(rayX, rayY)) {
            horiHitDis = P2P_DISTANCE(playerX, playerY, rayX, rayY);
            break;
        } else {
            rayX += xOffset;
            rayY += yOffset;
            depth += 1;
        }
    }

    // 選擇距離較短的命中點
    if (vertHitDis < horiHitDis) {
        // 垂直方向的命中
        rayX = vx;
        rayY = vy;
        *hitDirection = true;
        *hitOffset = rayY;
    } else {
        // 水平方向的命中
        *hitDirection = false;
        *hitOffset = rayX;
    }

    // 返回光線撞擊點到玩家位置的最小距離
    return fmin(vertHitDis, horiHitDis);
}

// 函數：RayCasterFloatTrace
// 參數：rayCaster - 光線追踪器
//       screenX - 螢幕上的 x 坐標
//       screenY - 螢幕上的 y 坐標
//       textureNo - 紋理編號
//       textureX - 紋理 x 坐標
//       textureY - 紋理 y 坐標
//       textureStep - 紋理步進值
// 說明：對浮點數光線追踪進行一次追踪，計算光線與牆面的交點及相關資訊
static void RayCasterFloatTrace(RayCaster *rayCaster,
                                uint16_t screenX,
                                uint8_t *screenY,
                                uint8_t *textureNo,
                                uint8_t *textureX,
                                uint16_t *textureY,
                                uint16_t *textureStep)
{
    float hitOffset;
    int hitDirection;

    // 計算光線的角度差
    // 計算視點到屏幕上每個像素的距離和方向
    float deltaAngle = atanf(((int16_t) screenX - SCREEN_WIDTH / 2.0f) /
                             (SCREEN_WIDTH / 2.0f) * M_PI / 4);

    // 使用浮點數距離函數計算光線的距離、偏移和方向
    float lineDistance = RayCasterFloatDistance(
        ((RayCasterFloat *) (rayCaster->derived))->playerX,
        ((RayCasterFloat *) (rayCaster->derived))->playerY,
        ((RayCasterFloat *) (rayCaster->derived))->playerA + deltaAngle,
        &hitOffset, &hitDirection);

    // 計算實際牆面距離
    // 計算真實距離（distance）和材質映射坐標（textureX）
    float distance = lineDistance * cos(deltaAngle);
    float dum;
    *textureX = (uint8_t) (256.0f * modff(hitOffset, &dum));
    *textureNo = hitDirection;
    *textureY = 0;
    *textureStep = 0;

    // 如果距離大於0，則進行進一步計算
    if (distance > 0) {
        // 計算螢幕上的投影高度
        // 計算屏幕上像素的垂直位置（screenY）
        float tmp = INV_FACTOR / distance;
        *screenY = tmp;

        // 計算材質的垂直映射和紋理步進（textureStep）
        float txs = (tmp * 2.0f);
        // 計算紋理步進值
        if (txs != 0) {
            *textureStep = (256 / txs) * 256;

            // 如果紋理投影超出螢幕高度，進行調整
            if (txs > SCREEN_HEIGHT) {
                float wallHeight = (txs - SCREEN_HEIGHT) / 2;
                *textureY = wallHeight * (256 / txs) * 256;
                *screenY = HORIZON_HEIGHT;
            }
        }
    } else {
        // 如果距離小於等於0，將螢幕上的投影高度設為0
        *screenY = 0;
    }
}

// 函數：RayCasterFloatStart
// 參數：rayCaster - 光線追踪器
//       playerX - 玩家的X座標
//       playerY - 玩家的Y座標
//       playerA - 玩家的角度
// 說明：初始化光線追踪器的起始位置和角度
static void RayCasterFloatStart(RayCaster *rayCaster,
                                uint16_t playerX,
                                uint16_t playerY,
                                int16_t playerA)
{
    // 將初始玩家位置和角度轉換為浮點數表示
    ((RayCasterFloat *) (rayCaster->derived))->playerX =
        (playerX / 1024.0f) * 4.0f;
    ((RayCasterFloat *) (rayCaster->derived))->playerY =
        (playerY / 1024.0f) * 4.0f;
    ((RayCasterFloat *) (rayCaster->derived))->playerA =
        (playerA / 1024.0f) * 2.0f * M_PI;
}

// 函數：RayCasterFloatDestruct
// 參數：rayCaster - 光線追踪器
// 說明：釋放光線追踪器的資源
static void RayCasterFloatDestruct(RayCaster *rayCaster)
{
    free(rayCaster->derived);
    RayCasterDestruct(rayCaster);
}
