#pragma once

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ROUND_DOWN(v, n) ((v) - ((v) % (n)))
#define ROUND_UP(v, n) ROUND_DOWN((v) + (n) - 1, n)

#define NULL ((void*)0)
