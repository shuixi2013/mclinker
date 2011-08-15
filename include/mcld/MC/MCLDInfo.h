/*****************************************************************************
 *   The MCLinker Project, Copyright (C), 2011 -                             *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   csmon7507 <csmon7507@gmail.com>                                         *
 ****************************************************************************/
#ifndef MCLDINFO_H
#define MCLDINFO_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <string>
#include <mcld/MC/MCLDOptions.h>
#include <mcld/Support/FileSystem.h>
#include <mcld/MC/MCLDInputTree.h>
#include <mcld/MC/InputFactory.h>
#include <mcld/MC/AttributeFactory.h>
#include <mcld/MC/ContextFactory.h>

namespace mcld
{

/** \class MCLDInfo
 *  \brief MCLDInfo is composed of argumments of MCLDDriver.
 *   options()        - the general options.
 *   inputs()         - the tree of inputs
 *   inputFactory()   - the list of all inputs
 *   attrFactory()    - the list of all attributes
 *   contextFactory() - the list of all contexts.
 */
class MCLDInfo
{
public:
  explicit MCLDInfo(size_t pAttrNum, size_t InputSize);
  virtual ~MCLDInfo();

  GeneralOptions& options()
  { return m_Options; }

  const GeneralOptions& options() const
  { return m_Options; }

  InputTree& inputs()
  { return *m_pInputTree; }

  const InputTree& inputs() const
  { return *m_pInputTree; }

  InputFactory& inputFactory()
  { return *m_pInputFactory; }

  const InputFactory& inputFactory() const
  { return *m_pInputFactory; }

  AttributeFactory& attrFactory()
  { return *m_pAttrFactory; }


  const AttributeFactory& attrFactory() const
  { return *m_pAttrFactory; }

  ContextFactory& contextFactory()
  { return *m_pCntxtFactory; }

  const ContextFactory& contextFactory() const
  { return *m_pCntxtFactory; }

private:
  GeneralOptions m_Options;
  InputTree *m_pInputTree;
  InputFactory *m_pInputFactory;
  AttributeFactory *m_pAttrFactory;
  ContextFactory *m_pCntxtFactory;

};

} // namespace of mcld

#endif

