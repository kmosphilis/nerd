#include "../../src/knowledge_base.h"

#ifndef __KNOWLEDGE_BASE_HELPER_H__
#define __KNOWLEDGE_BASE_HELPER_H__

void ck_assert_knowledge_base_eq(const KnowledgeBase * const knowledge_base1,
const KnowledgeBase * const knowledge_base2);
void ck_assert_knowledge_base_notempty(const KnowledgeBase * const knowledge_base);
void ck_assert_knowledge_base_empty(const KnowledgeBase * const knowledge_base);

#endif