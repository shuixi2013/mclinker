//===- LDContext.cpp ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/LDContext.h>
#include <mcld/LD/LDSection.h>
#include <mcld/LD/LDSymbol.h>

using namespace mcld;

//==========================
// LDReader
LDContext::LDContext()
{
}

LDContext::~LDContext()
{
}

LDSection* LDContext::getSection(unsigned int pIdx)
{
  return m_SectionTable.at(pIdx);
}

const LDSection* LDContext::getSection(unsigned int pIdx) const
{
  return m_SectionTable.at(pIdx);
}

LDSection* LDContext::getSection(const std::string& pName)
{
  sect_iterator sect_iter, sect_end = sectEnd();
  for (sect_iter = sectBegin(); sect_iter != sect_end; ++sect_iter) {
    if((*sect_iter)->name() == pName)
      return *sect_iter;
  }
  return NULL;
}

const LDSection* LDContext::getSection(const std::string& pName) const
{
  const_sect_iterator sect_iter, sect_end = sectEnd();
  for (sect_iter = sectBegin(); sect_iter != sect_end; ++sect_iter) {
    if((*sect_iter)->name() == pName)
      return *sect_iter;
  }
  return NULL;
}

