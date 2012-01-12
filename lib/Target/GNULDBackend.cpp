//===- GNULDBackend.cpp ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <llvm/Support/ELF.h>
#include <mcld/ADT/SizeTraits.h>
#include <mcld/Target/GNULDBackend.h>
#include <mcld/MC/MCLDInfo.h>
#include <mcld/MC/MCLDOutput.h>
#include <mcld/MC/MCLDInputTree.h>
#include <mcld/LD/LDSymbol.h>
#include <mcld/LD/Layout.h>
#include <mcld/Support/MemoryArea.h>
#include <mcld/Support/MemoryRegion.h>
#include <string>
#include <cassert>

using namespace mcld;

//===----------------------------------------------------------------------===//
// non-member functions
namespace {

unsigned int
hash_bucket_count(unsigned int pNumOfSymbols, bool pIsGNUStyle)
{
  // @ref Google gold, dynobj.cc:loc 791
  static const unsigned int buckets[] =
  {
    1, 3, 17, 37, 67, 97, 131, 197, 263, 521, 1031, 2053, 4099, 8209,
    16411, 32771, 65537, 131101, 262147
  };
  const unsigned buckets_count = sizeof buckets / sizeof buckets[0];

  unsigned int result = 1;
  for (unsigned i = 0; i < buckets_count; ++i) {
    if (pNumOfSymbols < buckets[i])
      break;
    result = buckets[i];
  }

  if (pIsGNUStyle && result < 2)
    result = 2;

  return result;
}

// isDynamicSymbol
// @ref Google gold linker: symtab.cc: 311
inline bool isDynamicSymbol(const LDSymbol& pSymbol, const Output& pOutput)
{
  // If the symbol is used in backend (usually used by dynamic relocation),
  // we need to add it.
  if (0x0 != pSymbol.resolveInfo()->reserved())
    return true;

  // If a local symbol is in the LDContext's symbol table, it's a real local
  // symbol. We should not add it
  if (pSymbol.binding() == ResolveInfo::Local)
    return false;

  // If we are building shared object, and the visibility is external, we
  // need to add it.
  if (Output::DynObj == pOutput.type())
    if (pSymbol.resolveInfo()->visibility() == ResolveInfo::Default ||
        pSymbol.resolveInfo()->visibility() == ResolveInfo::Protected)
      return true;

  return false;
}

} // end namespace

//===----------------------------------------------------------------------===//
// GNULDBackend
GNULDBackend::GNULDBackend()
  : m_pArchiveReader(0),
    m_pObjectReader(0),
    m_pDynObjReader(0),
    m_pObjectWriter(0),
    m_pDynObjWriter(0),
    m_pDynObjFileFormat(0),
    m_pExecFileFormat(0),
    m_ELFSegmentFactory(9), // magic number
    m_pDynamic(0) {
}

GNULDBackend::~GNULDBackend()
{
  if (m_pArchiveReader)
    delete m_pArchiveReader;
  if (m_pObjectReader)
    delete m_pObjectReader;
  if (m_pDynObjReader)
    delete m_pDynObjReader;
  if (m_pObjectWriter)
    delete m_pObjectWriter;
  if (m_pDynObjWriter)
    delete m_pDynObjWriter;
  if (m_pDynObjFileFormat)
    delete m_pDynObjFileFormat;
  if (m_pExecFileFormat)
    delete m_pExecFileFormat;
  if (m_pDynamic)
    delete m_pDynamic;
}

size_t GNULDBackend::sectionStartOffset() const
{
  // FIXME: use fixed offset, we need 10 segments by default
  return sizeof(llvm::ELF::Elf64_Ehdr)+10*sizeof(llvm::ELF::Elf64_Phdr);
}

bool GNULDBackend::initArchiveReader(MCLinker&)
{
  if (0 == m_pArchiveReader)
    // FIXME: What is the correct ctor for GNUArchiveReader?
    m_pArchiveReader = new GNUArchiveReader();
  return true;
}

bool GNULDBackend::initObjectReader(MCLinker& pLinker)
{
  if (0 == m_pObjectReader)
    m_pObjectReader = new ELFObjectReader(*this, pLinker);
  return true;
}

bool GNULDBackend::initDynObjReader(MCLinker& pLinker)
{
  if (0 == m_pDynObjReader)
    m_pDynObjReader = new ELFDynObjReader(*this, pLinker);
  return true;
}

bool GNULDBackend::initObjectWriter(MCLinker&)
{
  // TODO
  return true;
}

bool GNULDBackend::initDynObjWriter(MCLinker& pLinker)
{
  if (0 == m_pDynObjWriter)
    m_pDynObjWriter = new ELFDynObjWriter(*this, pLinker);
  return true;
}

bool GNULDBackend::initExecSections(MCLinker& pMCLinker)
{
  if (0 == m_pExecFileFormat)
    m_pExecFileFormat = new ELFExecFileFormat();

  // initialize standard sections
  m_pExecFileFormat->initStdSections(pMCLinker);
  return true;
}

bool GNULDBackend::initDynObjSections(MCLinker& pMCLinker)
{
  if (0 == m_pDynObjFileFormat)
    m_pDynObjFileFormat = new ELFDynObjFileFormat();

  // initialize standard sections
  m_pDynObjFileFormat->initStdSections(pMCLinker);
  return true;
}

GNUArchiveReader *GNULDBackend::getArchiveReader()
{
  assert(0 != m_pArchiveReader);
  return m_pArchiveReader;
}

GNUArchiveReader *GNULDBackend::getArchiveReader() const
{
  assert(0 != m_pArchiveReader);
  return m_pArchiveReader;
}

ELFObjectReader *GNULDBackend::getObjectReader()
{
  assert(0 != m_pObjectReader);
  return m_pObjectReader;
}

ELFObjectReader *GNULDBackend::getObjectReader() const
{
  assert(0 != m_pObjectReader);
  return m_pObjectReader;
}

ELFDynObjReader *GNULDBackend::getDynObjReader()
{
  assert(0 != m_pDynObjReader);
  return m_pDynObjReader;
}

ELFDynObjReader *GNULDBackend::getDynObjReader() const
{
  assert(0 != m_pDynObjReader);
  return m_pDynObjReader;
}

ELFObjectWriter *GNULDBackend::getObjectWriter()
{
  // TODO
  return NULL;
}

ELFObjectWriter *GNULDBackend::getObjectWriter() const
{
  // TODO
  return NULL;
}

ELFDynObjWriter *GNULDBackend::getDynObjWriter()
{
  assert(0 != m_pDynObjWriter);
  return m_pDynObjWriter;
}

ELFDynObjWriter *GNULDBackend::getDynObjWriter() const
{
  assert(0 != m_pDynObjWriter);
  return m_pDynObjWriter;
}


ELFDynObjFileFormat* GNULDBackend::getDynObjFileFormat()
{
  assert(0 != m_pDynObjFileFormat);
  return m_pDynObjFileFormat;
}

ELFDynObjFileFormat* GNULDBackend::getDynObjFileFormat() const
{
  assert(0 != m_pDynObjFileFormat);
  return m_pDynObjFileFormat;
}

ELFExecFileFormat* GNULDBackend::getExecFileFormat()
{
  assert(0 != m_pExecFileFormat);
  return m_pExecFileFormat;
}

ELFExecFileFormat* GNULDBackend::getExecFileFormat() const
{
  assert(0 != m_pExecFileFormat);
  return m_pExecFileFormat;
}

/// sizeNamePools - compute the size of regular name pools
/// In ELF executable files, regular name pools are .symtab, .strtab.,
/// .dynsym, .dynstr, and .hash
void
GNULDBackend::sizeNamePools(const Output& pOutput,
                            const MCLDInfo& pLDInfo)
{
  size_t symtab = 0;
  size_t dynsym = 1;
  size_t strtab = 0;
  size_t dynstr = 1;
  size_t hash   = 0;

  // compute size of .symtab, .dynsym and .strtab
  LDContext::const_sym_iterator symbol;
  LDContext::const_sym_iterator symEnd = pOutput.context()->symEnd();
  for (symbol = pOutput.context()->symBegin(); symbol != symEnd; ++symbol) {
    size_t str_size = (*symbol)->nameSize() + 1;
    if (isDynamicSymbol(**symbol, pOutput)) {
      ++dynsym;
      dynstr += str_size;
    }
    ++symtab;
    strtab += str_size;
  }

  ELFFileFormat* file_format = NULL;
  switch(pOutput.type()) {
    // compute size of .dynstr and .hash
    case Output::DynObj:
      file_format = getDynObjFileFormat();
      break;
    case Output::Exec:
      file_format = getExecFileFormat();
      break;
    case Output::Object:
    default:
      // TODO: not support yet
      return;
  }

  switch(pOutput.type()) {
    // compute size of .dynstr and .hash
    case Output::DynObj:
    case Output::Exec: {
      // create .dynamic section
      m_pDynamic = new ELFDynamic(*this);

      // add DT_NEED strings into .dynstr and .dynamic
      // Rules:
      //   1. ignore --no-add-needed
      //   2. force count in --no-as-needed
      //   3. judge --as-needed
      InputTree::const_bfs_iterator input, inputEnd = pLDInfo.inputs().bfs_end();
      for (input = pLDInfo.inputs().bfs_begin(); input != inputEnd; ++input) {
        if (Input::DynObj == (*input)->type()) {
          // --add-needed
          if ((*input)->attribute()->isAddNeeded()) {
            // --no-as-needed
            if (!(*input)->attribute()->isAsNeeded()) {
              dynstr += (*input)->name().size() + 1;
              m_pDynamic->reserveNeedEntry();
            }
            // --as-needed
            else if ((*input)->isNeeded()) {
              dynstr += (*input)->name().size() + 1;
              m_pDynamic->reserveNeedEntry();
            }
          }
        }
      } // for

      // compute .hash
      // Both Elf32_Word and Elf64_Word are 4 bytes
      hash = (2 + hash_bucket_count((dynsym-1), false) + (dynsym-1)) * sizeof(llvm::ELF::Elf32_Word);

      // set size
      dynstr += pOutput.name().size() + 1;
      if (32 == bitclass())
        file_format->getDynSymTab().setSize(dynsym*sizeof(llvm::ELF::Elf32_Sym));
      else
        file_format->getDynSymTab().setSize(dynsym*sizeof(llvm::ELF::Elf64_Sym));
      file_format->getDynStrTab().setSize(dynstr);
      file_format->getHashTab().setSize(hash);

    }
    /* fall through */
    case Output::Object: {
      if (32 == bitclass())
        file_format->getSymTab().setSize(symtab*sizeof(llvm::ELF::Elf32_Sym));
      else
        file_format->getSymTab().setSize(symtab*sizeof(llvm::ELF::Elf64_Sym));
      file_format->getStrTab().setSize(strtab);
      break;
    }
  } // end of switch

  // reserve fixed entries in the .dynamic section.
  if (Output::DynObj == pOutput.type() || Output::Exec == pOutput.type()) {
    // Because some entries in .dynamic section need information of .dynsym,
    // .dynstr, .symtab, .strtab and .hash, we can not reserve non-DT_NEEDED
    // entries untill we get the size of the abovementioned sections
    m_pDynamic->reserveEntries(pLDInfo, *file_format);
    file_format->getDynamic().setSize(m_pDynamic->numOfBytes());
  }
}

/// emitRegNamePools - emit regular name pools - .symtab, .strtab
///
/// the size of these tables should be computed before layout
/// layout should computes the start offset of these tables
void GNULDBackend::emitRegNamePools(Output& pOutput,
                                    const Layout& pLayout,
                                    const MCLDInfo& pLDInfo)
{

  assert(pOutput.hasMemArea());
  ELFFileFormat* file_format = NULL;
  switch(pOutput.type()) {
    // compute size of .dynstr and .hash
    case Output::DynObj:
      file_format = getDynObjFileFormat();
      break;
    case Output::Exec:
      file_format = getExecFileFormat();
      break;
    case Output::Object:
    default:
      // TODO: not support yet
      return;
  }

  LDSection& symtab_sect = file_format->getSymTab();
  LDSection& strtab_sect = file_format->getStrTab();

  MemoryRegion* symtab_region = pOutput.memArea()->request(symtab_sect.offset(),
                                                           symtab_sect.size(),
                                                           true);
  MemoryRegion* strtab_region = pOutput.memArea()->request(strtab_sect.offset(),
                                                           strtab_sect.size(),
                                                           true);

  // set up symtab_region
  llvm::ELF::Elf32_Sym* symtab32 = NULL;
  llvm::ELF::Elf64_Sym* symtab64 = NULL;
  if (32 == bitclass())
    symtab32 = (llvm::ELF::Elf32_Sym*)symtab_region->start();
  else if (64 == bitclass())
    symtab64 = (llvm::ELF::Elf64_Sym*)symtab_region->start();
  else
    llvm::report_fatal_error(llvm::Twine("unsupported bitclass ") +
                             llvm::Twine(bitclass()) +
                             llvm::Twine(".\n"));
  // set up strtab_region
  char* strtab = (char*)strtab_region->start();
  strtab[0] = '\0';

  size_t symtabIdx = 0;
  size_t strtabsize = 0;
  // compute size of .symtab, .dynsym and .strtab
  LDContext::const_sym_iterator symbol;
  LDContext::const_sym_iterator symEnd = pOutput.context()->symEnd();
  for (symbol = pOutput.context()->symBegin(); symbol != symEnd; ++symbol) {
    // FIXME: check the endian between host and target
    // write out symbol
    if (32 == bitclass()) {
      symtab32[symtabIdx].st_name  = strtabsize;
      symtab32[symtabIdx].st_value = (*symbol)->value();
      symtab32[symtabIdx].st_size  = (*symbol)->size();
      symtab32[symtabIdx].st_info  = getSymbolInfo(**symbol);
      symtab32[symtabIdx].st_other = (*symbol)->visibility();
      symtab32[symtabIdx].st_shndx = getSymbolShndx(**symbol, pLayout);
    }
    else { // must 64
      symtab64[symtabIdx].st_name  = strtabsize;
      symtab64[symtabIdx].st_value = (*symbol)->value();
      symtab64[symtabIdx].st_size  = (*symbol)->size();
      symtab64[symtabIdx].st_info  = getSymbolInfo(**symbol);
      symtab64[symtabIdx].st_other = (*symbol)->visibility();
      symtab64[symtabIdx].st_shndx = getSymbolShndx(**symbol, pLayout);
    }
    // write out string
    strcpy((strtab + strtabsize), (*symbol)->name());

    // write out 
    // sum up counters
    ++symtabIdx;
    strtabsize += (*symbol)->nameSize() + 1;
  }

  symtab_region->sync();
  strtab_region->sync();
}

/// emitNamePools - emit dynamic name pools - .dyntab, .dynstr, .hash
///
/// the size of these tables should be computed before layout
/// layout should computes the start offset of these tables
void GNULDBackend::emitDynNamePools(Output& pOutput,
                                    const Layout& pLayout,
                                    const MCLDInfo& pLDInfo)
{
  assert(pOutput.hasMemArea());
  ELFFileFormat* file_format = NULL;
  switch(pOutput.type()) {
    // compute size of .dynstr and .hash
    case Output::DynObj:
      file_format = getDynObjFileFormat();
      break;
    case Output::Exec:
      file_format = getExecFileFormat();
      break;
    case Output::Object:
    default:
      // TODO: not support yet
      return;
  }

  LDSection& symtab_sect = file_format->getDynSymTab();
  LDSection& strtab_sect = file_format->getDynStrTab();
  LDSection& hash_sect   = file_format->getHashTab();
  LDSection& dyn_sect    = file_format->getDynamic();

  MemoryRegion* symtab_region = pOutput.memArea()->request(symtab_sect.offset(),
                                                           symtab_sect.size(),
                                                           true);
  MemoryRegion* strtab_region = pOutput.memArea()->request(strtab_sect.offset(),
                                                           strtab_sect.size(),
                                                           true);
  MemoryRegion* hash_region = pOutput.memArea()->request(hash_sect.offset(),
                                                         hash_sect.size(),
                                                         true);
  MemoryRegion* dyn_region = pOutput.memArea()->request(dyn_sect.offset(),
                                                        dyn_sect.size(),
                                                        true);
  // set up symtab_region
  llvm::ELF::Elf32_Sym* symtab32 = NULL;
  llvm::ELF::Elf64_Sym* symtab64 = NULL;
  if (32 == bitclass())
    symtab32 = (llvm::ELF::Elf32_Sym*)symtab_region->start();
  else if (64 == bitclass())
    symtab64 = (llvm::ELF::Elf64_Sym*)symtab_region->start();
  else
    llvm::report_fatal_error(llvm::Twine("unsupported bitclass ") +
                             llvm::Twine(bitclass()) +
                             llvm::Twine(".\n"));

  // initialize the first ELF symbol
  if (32 == bitclass()) {
    symtab32[0].st_name  = 0;
    symtab32[0].st_value = 0;
    symtab32[0].st_size  = 0;
    symtab32[0].st_info  = 0;
    symtab32[0].st_other = 0;
    symtab32[0].st_shndx = 0;
  }
  else { // must 64
    symtab64[0].st_name  = 0;
    symtab64[0].st_value = 0;
    symtab64[0].st_size  = 0;
    symtab64[0].st_info  = 0;
    symtab64[0].st_other = 0;
    symtab64[0].st_shndx = 0;
  }
  // set up strtab_region
  char* strtab = (char*)strtab_region->start();
  strtab[0] = '\0';

  size_t symtabIdx = 1;
  size_t strtabsize = 1;

  // emit of .dynsym, and .dynstr
  LDContext::const_sym_iterator symbol;
  LDContext::const_sym_iterator symEnd = pOutput.context()->symEnd();
  for (symbol = pOutput.context()->symBegin(); symbol != symEnd; ++symbol) {
    if (!isDynamicSymbol(**symbol, pOutput))
      continue;
    // FIXME: check the endian between host and target
    // write out symbol
    if (32 == bitclass()) {
      symtab32[symtabIdx].st_name  = strtabsize;
      symtab32[symtabIdx].st_value = (*symbol)->value();
      symtab32[symtabIdx].st_size  = (*symbol)->size();
      symtab32[symtabIdx].st_info  = getSymbolInfo(**symbol);
      symtab32[symtabIdx].st_other = (*symbol)->visibility();
      symtab32[symtabIdx].st_shndx = getSymbolShndx(**symbol, pLayout);
    }
    else { // must 64
      symtab64[symtabIdx].st_name  = strtabsize;
      symtab64[symtabIdx].st_value = (*symbol)->value();
      symtab64[symtabIdx].st_size  = (*symbol)->size();
      symtab64[symtabIdx].st_info  = getSymbolInfo(**symbol);
      symtab64[symtabIdx].st_other = (*symbol)->visibility();
      symtab64[symtabIdx].st_shndx = getSymbolShndx(**symbol, pLayout);
    }
    // write out string
    strcpy((strtab + strtabsize), (*symbol)->name());

    // sum up counters
    ++symtabIdx;
    strtabsize += (*symbol)->nameSize() + 1;
  }

  // emit DT_NEED
  // add DT_NEED strings into .dynstr
  // Rules:
  //   1. ignore --no-add-needed
  //   2. force count in --no-as-needed
  //   3. judge --as-needed
  ELFDynamic::iterator dt_need = m_pDynamic->needBegin();
  InputTree::const_bfs_iterator input, inputEnd = pLDInfo.inputs().bfs_end();
  for (input = pLDInfo.inputs().bfs_begin(); input != inputEnd; ++input) {
    if (Input::DynObj == (*input)->type()) {
      // --add-needed
      if ((*input)->attribute()->isAddNeeded()) {
        // --no-as-needed
        if (!(*input)->attribute()->isAsNeeded()) {
          strcpy((strtab + strtabsize), (*input)->name().c_str());
          (*dt_need)->setValue(llvm::ELF::DT_NEEDED, strtabsize);
          strtabsize += (*input)->name().size() + 1;
          ++dt_need;
        }
        // --as-needed
        else if ((*input)->isNeeded()) {
          strcpy((strtab + strtabsize), (*input)->name().c_str());
          (*dt_need)->setValue(llvm::ELF::DT_NEEDED, strtabsize);
          strtabsize += (*input)->name().size() + 1;
          ++dt_need;
        }
      }
    }
  } // for

  // emit soname
  // initialize value of ELF .dynamic section
  m_pDynamic->applySoname(strtabsize);
  m_pDynamic->applyEntries(pLDInfo, *file_format);
  m_pDynamic->emit(dyn_sect, *dyn_region);

  strcpy((strtab + strtabsize), pOutput.name().c_str());
  strtabsize += pOutput.name().size() + 1;

  // emit hash table
  // FIXME: this verion only emit SVR4 hash section.
  //        Please add GNU new hash section

  // both 32 and 64 bits hash table use 32-bit entry
  // set up hash_region
  uint32_t* word_array = (uint32_t*)hash_region->start();
  uint32_t& nbucket = word_array[0];
  uint32_t& nchain  = word_array[1];

  nbucket = hash_bucket_count(symtabIdx, false);
  nchain  = symtabIdx;

  uint32_t* bucket = (word_array + 2);
  uint32_t* chain  = (bucket + nbucket);

  // initialize bucket
  bzero((void*)bucket, nbucket);

  StringHash<ELF> hash_func;

  if (32 == bitclass()) {
    for (size_t sym_idx=0; sym_idx < symtabIdx; ++sym_idx) {
      llvm::StringRef name(strtab + symtab32[sym_idx].st_name);
      size_t bucket_pos = hash_func(name) % nbucket;
      chain[sym_idx] = bucket[bucket_pos];
      bucket[bucket_pos] = sym_idx;
    }
  }
  else if (64 == bitclass()) {
    for (size_t sym_idx=0; sym_idx < symtabIdx; ++sym_idx) {
      llvm::StringRef name(strtab + symtab64[sym_idx].st_name);
      size_t bucket_pos = hash_func(name) % nbucket;
      chain[sym_idx] = bucket[bucket_pos];
      bucket[bucket_pos] = sym_idx;
    }
  }

  symtab_region->sync();
  strtab_region->sync();
  hash_region->sync();
  dyn_region->sync();
}

/// getSectionOrder
unsigned int GNULDBackend::getSectionOrder(const LDSection& pSectHdr) const
{
  // NULL section should be the "1st" section
  if (LDFileFormat::Null == pSectHdr.kind())
    return 0;

  // if the section is not ALLOC, lay it out until the last possible moment
  if (0 == (pSectHdr.flag() & llvm::ELF::SHF_ALLOC))
    return SHO_UNDEFINED;

  bool is_write = (pSectHdr.flag() & llvm::ELF::SHF_WRITE) != 0;
  bool is_exec = (pSectHdr.flag() & llvm::ELF::SHF_EXECINSTR) != 0;

  // TODO: need to take care other possible output sections
  switch (pSectHdr.kind()) {
    case LDFileFormat::Regular:
      if (is_exec) {
        if (pSectHdr.name() == ".init")
          return SHO_INIT;
        else if (pSectHdr.name() == ".fini")
          return SHO_FINI;
        else
          return SHO_TEXT;
      } else if (!is_write) {
        return SHO_RO;
      } else {
        if (pSectHdr.type() == llvm::ELF::SHT_PREINIT_ARRAY ||
            pSectHdr.type() == llvm::ELF::SHT_INIT_ARRAY ||
            pSectHdr.type() == llvm::ELF::SHT_FINI_ARRAY ||
            pSectHdr.name() == ".ctors" ||
            pSectHdr.name() == ".dtors")
          return SHO_RELRO;

        return SHO_DATA;
      }

    case LDFileFormat::BSS:
      return SHO_BSS;

    case LDFileFormat::NamePool:
      if (pSectHdr.name() == ".dynamic")
        return SHO_RELRO;
      return SHO_NAMEPOOL;

    case LDFileFormat::Relocation:
      if (std::string::npos != pSectHdr.name().find("plt"))
        return SHO_REL_PLT;
      return SHO_RELOCATION;

    // get the order from target for target specific sections
    case LDFileFormat::Target:
      return getTargetSectionOrder(pSectHdr);

    // handle .interp
    case LDFileFormat::Note:
      return SHO_INTERP;

    case LDFileFormat::MetaData:
    case LDFileFormat::Debug:
    default:
      return SHO_UNDEFINED;
  }
}

/// getSymbolInfo
uint64_t GNULDBackend::getSymbolInfo(const LDSymbol& pSymbol) const
{
  // set binding
  uint8_t bind = 0x0;
  if (pSymbol.resolveInfo()->isLocal())
    bind = llvm::ELF::STB_LOCAL;
  else if (pSymbol.resolveInfo()->isGlobal())
    bind = llvm::ELF::STB_GLOBAL;
  else if (pSymbol.resolveInfo()->isWeak())
    bind = llvm::ELF::STB_WEAK;

  return (pSymbol.resolveInfo()->type() | (bind << 4));
}

/// getSymbolShndx
uint64_t
GNULDBackend::getSymbolShndx(const LDSymbol& pSymbol, const Layout& pLayout) const
{
  if (pSymbol.resolveInfo()->isAbsolute())
    return llvm::ELF::SHN_ABS;
  if (pSymbol.resolveInfo()->isCommon())
    return llvm::ELF::SHN_COMMON;
  if (pSymbol.resolveInfo()->isUndef())
    return llvm::ELF::SHN_UNDEF;

  if (pSymbol.resolveInfo()->isLocal()) {
    switch (pSymbol.type()) {
      case ResolveInfo::NoType:
      case ResolveInfo::File:
        return llvm::ELF::SHN_ABS;
    }
  }
  assert(pSymbol.hasFragRef());
  return pLayout.getOutputLDSection(*pSymbol.fragRef()->frag())->index();
}

/// emitProgramHdrs - emit ELF program headers
void GNULDBackend::emitProgramHdrs(const Output& pOutput)
{
  assert(NULL != pOutput.context());
  createProgramHdrs(const_cast<LDContext&>(*pOutput.context()));

  if (32 == bitclass())
    writeELF32ProgramHdrs(const_cast<Output&>(pOutput));
  else
    writeELF64ProgramHdrs(const_cast<Output&>(pOutput));
}

/// createProgramHdrs - base on output sections to create the program headers
void GNULDBackend::createProgramHdrs(LDContext& pContext)
{
  // make PT_PHDR
  ELFSegment* phdr_seg = m_ELFSegmentFactory.allocate();
  new (phdr_seg) ELFSegment(llvm::ELF::PT_PHDR, llvm::ELF::PF_R);

  // make PT_INTERP
  LDSection* interp = pContext.getSection(".interp");
  if (NULL != interp) {
    ELFSegment* interp_seg = m_ELFSegmentFactory.allocate();
    new (interp_seg) ELFSegment(llvm::ELF::PT_INTERP, llvm::ELF::PF_R);
    interp_seg->addSection(interp);
    interp_seg->setAlign(bitclass() / 8);
  }

  uint32_t cur_seg_flag, prev_seg_flag = getSegmentFlag(0);
  uint64_t padding = 0;
  ELFSegment* load_seg;
  // make possible PT_LOAD segments
  LDContext::sect_iterator sect, sect_end = pContext.sectEnd();
  for (sect = pContext.sectBegin(); sect != sect_end; ++sect) {
    if (0 == ((*sect)->flag() & llvm::ELF::SHF_ALLOC) &&
        LDFileFormat::Null != (*sect)->kind())
      continue;

    // FIXME: Now only separate writable and non-writable PT_LOAD
    cur_seg_flag = getSegmentFlag((*sect)->flag());
    if ((prev_seg_flag & llvm::ELF::PF_W) ^ (cur_seg_flag & llvm::ELF::PF_W) ||
         LDFileFormat::Null == (*sect)->kind()) {
      // create new PT_LOAD segment
      load_seg = m_ELFSegmentFactory.allocate();
      new (load_seg) ELFSegment(llvm::ELF::PT_LOAD);
      load_seg->setAlign(pagesize());

      // check if this segment needs padding
      padding = 0;
      if (((*sect)->offset() & (load_seg->align() - 1)) != 0)
        padding = load_seg->align();
    }

    load_seg->addSection(*sect);
    load_seg->updateFlag(cur_seg_flag);

    // FIXME: set section's vma
    // need to handle start vma for user-defined one or for executable.
    (*sect)->setAddr((*sect)->offset() + padding);

    prev_seg_flag = cur_seg_flag;
  }

  // make PT_DYNAMIC
  LDSection* dynamic = pContext.getSection(".dynamic");
  if (NULL != dynamic) {
    ELFSegment* dyn_seg = m_ELFSegmentFactory.allocate();
    new (dyn_seg) ELFSegment(llvm::ELF::PT_DYNAMIC,
                             llvm::ELF::PF_R | llvm::ELF::PF_W);
    dyn_seg->addSection(dynamic);
    dyn_seg->setAlign(bitclass() / 8);
  }

  // update segment info
  bool is_first_pt_load = true;
  uint64_t file_size = 0;
  ELFSegmentFactory::iterator seg, seg_end = m_ELFSegmentFactory.end();
  for (seg = m_ELFSegmentFactory.begin(); seg != seg_end; ++seg) {
    ELFSegment& segment = *seg;

    // update PT_PHDR
    if (llvm::ELF::PT_PHDR == segment.type()) {
      uint64_t offset, phdr_size;
      if (32 == bitclass()) {
        offset = sizeof(llvm::ELF::Elf32_Ehdr);
        phdr_size = sizeof(llvm::ELF::Elf32_Phdr);
      }
      else {
        offset = sizeof(llvm::ELF::Elf64_Ehdr);
        phdr_size = sizeof(llvm::ELF::Elf64_Phdr);
      }
      segment.setOffset(offset);
      segment.setVaddr(offset);
      segment.setPaddr(segment.vaddr());
      segment.setFilesz(numOfSegments() * phdr_size);
      segment.setMemsz(numOfSegments() * phdr_size);
      segment.setAlign(bitclass() / 8);
      continue;
    }

    assert(NULL != segment.getFirstSection());
    segment.setOffset(segment.getFirstSection()->offset());
    segment.setVaddr(segment.getFirstSection()->addr());
    segment.setPaddr(segment.vaddr());

    // 1st PT_LOAD should include ELF file header and program headers
    if (llvm::ELF::PT_LOAD == segment.type() && is_first_pt_load) {
      assert(NULL != segment.getLastSection());
      file_size = segment.getLastSection()->addr()
                  + segment.getLastSection()->size()
                  - segment.vaddr();
      is_first_pt_load = false;
    } else {
      file_size = 0;
      ELFSegment::sect_iterator sect, sect_end = segment.sectEnd();
      for (sect = segment.sectBegin(); sect != sect_end; ++sect) {
        if (LDFileFormat::BSS != (*sect)->kind())
          file_size += (*sect)->size();
      }
    }
    segment.setFilesz(file_size);

    assert(NULL != segment.getLastSection());
    segment.setMemsz(segment.getLastSection()->addr()
                     + segment.getLastSection()->size()
                     - segment.vaddr());
  }
}

/// writeELF32ProgramHdrs - write out the ELF32 program headers
void GNULDBackend::writeELF32ProgramHdrs(Output& pOutput)
{
  assert(pOutput.hasMemArea());

  uint64_t start_offset, phdr_size;

  start_offset = sizeof(llvm::ELF::Elf32_Ehdr);
  phdr_size = sizeof(llvm::ELF::Elf32_Phdr);
  // Program header must start directly after ELF header
  MemoryRegion *region = pOutput.memArea()->request(start_offset,
                                                    numOfSegments() *phdr_size,
                                                    true);
  llvm::ELF::Elf32_Phdr* phdr = (llvm::ELF::Elf32_Phdr*)region->start();

  size_t index = 0;
  ELFSegmentFactory::iterator seg, segEnd = m_ELFSegmentFactory.end();
  for (seg = m_ELFSegmentFactory.begin(); seg != segEnd; ++seg, ++index) {
    phdr[index].p_type   = (*seg).type();
    phdr[index].p_flags  = (*seg).flag();
    phdr[index].p_offset = (*seg).offset();
    phdr[index].p_vaddr  = (*seg).vaddr();
    phdr[index].p_paddr  = (*seg).paddr();
    phdr[index].p_filesz = (*seg).filesz();
    phdr[index].p_memsz  = (*seg).memsz();
    phdr[index].p_align  = (*seg).align();
  }
  region->sync();
}

/// writeELF64ProgramHdrs - write out the ELF64 program headers
void GNULDBackend::writeELF64ProgramHdrs(Output& pOutput)
{
  assert(pOutput.hasMemArea());

  uint64_t start_offset, phdr_size;

  start_offset = sizeof(llvm::ELF::Elf64_Ehdr);
  phdr_size = sizeof(llvm::ELF::Elf64_Phdr);
  // Program header must start directly after ELF header
  MemoryRegion *region = pOutput.memArea()->request(start_offset,
                                                    numOfSegments() *phdr_size,
                                                    true);
  llvm::ELF::Elf64_Phdr* phdr = (llvm::ELF::Elf64_Phdr*)region->start();

  size_t index = 0;
  ELFSegmentFactory::iterator seg, segEnd = m_ELFSegmentFactory.end();
  for (seg = m_ELFSegmentFactory.begin(); seg != segEnd; ++seg, ++index) {
    phdr[index].p_type   = (*seg).type();
    phdr[index].p_flags  = (*seg).flag();
    phdr[index].p_offset = (*seg).offset();
    phdr[index].p_vaddr  = (*seg).vaddr();
    phdr[index].p_paddr  = (*seg).paddr();
    phdr[index].p_filesz = (*seg).filesz();
    phdr[index].p_memsz  = (*seg).memsz();
    phdr[index].p_align  = (*seg).align();
  }
  region->sync();
}

/// preLayout - Backend can do any needed modification before layout
void GNULDBackend::preLayout(const Output& pOutput,
                             const MCLDInfo& pLDInfo,
                             MCLinker& pLinker)
{
  // prelayout target first
  doPreLayout(pOutput, pLDInfo, pLinker);
}

/// postLayout -Backend can do any needed modification after layout
void GNULDBackend::postLayout(const Output& pOutput,
                              const MCLDInfo& pInfo,
                              MCLinker& pLinker)
{
  // post layout target first
  doPostLayout(pOutput, pInfo, pLinker);
}

