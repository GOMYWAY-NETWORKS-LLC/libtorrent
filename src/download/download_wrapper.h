#ifndef LIBTORRENT_DOWNLOAD_WRAPPER_H
#define LIBTORRENT_DOWNLOAD_WRAPPER_H

#include <memory>

#include "download_main.h"

namespace torrent {

// Remember to clean up the pointers, DownloadWrapper won't do it.

class Bencode;
class HashTorrent;
class HashQueue;
class HandshakeManager;

class DownloadWrapper {
public:
  DownloadWrapper();
  ~DownloadWrapper();

  // Initialize hash checker and various download stuff.
  void                initialize(const std::string& hash, const std::string& id);

  // Don't load unless the object is newly initialized.
  void                resume_load();
  void                resume_save();

  void                open();
  void                stop();

  const std::string&  get_hash()          { return m_main.get_hash(); }
  DownloadMain&       get_main()          { return m_main; }

  Bencode&            get_bencode()       { return *m_bencode.get(); }
  HashTorrent&        get_hash_checker()  { return *m_hash.get(); }

  void                set_handshake_manager(HandshakeManager& h);
  void                set_hash_queue(HashQueue& h);

  // Various functions for manipulating bencode's data with the
  // download.
  
private:
  DownloadWrapper(const DownloadWrapper&);
  void operator = (const DownloadWrapper&);

  DownloadMain               m_main;
  std::auto_ptr<Bencode>     m_bencode;
  std::auto_ptr<HashTorrent> m_hash;
};

}

#endif