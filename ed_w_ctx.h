#ifndef ED_W_CTX_H
#define ED_W_CTX_H

#include "ed_com.h"

void ed_WorldContextInit();

void ed_WorldContextShutdown();

void ed_WorldContextFlyCamera();

void ed_WorldContextDrawGrid();

void ed_WorldContextDrawBrushes();

void ed_WorldContextDrawLights();

void ed_WorldContextDrawSelections();

void ed_WorldContextUpdate();

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextProcessSelection(struct ed_context_t *context, uint32_t just_changed);


#endif // ED_W_CTX_H
