#ifndef MFCDNS_H
#define MFCDNS_H

#include <string>
#include <map>

#include <boost/thread.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

using namespace std;


#include "pubkey.h"


#define MFCDNS_PORT		5335
#define MFCDNS_DAPBLOOMSTEP	3				// 3 steps in bloom filter
#define MFCDNS_DAPSHIFTDECAY	8				// Dap time shift 8 = 256 secs (~4min) in decay
#define MFCDNS_DAPTRESHOLD	(4 << MFCDNS_DAPSHIFTDECAY)	// ~4r/s found name, ~1 r/s - clien IP

#define VERMASK_NEW	-1
#define VERMASK_BLOCKED -2
#define VERMASK_NOSRL	(1 << 24)	// ENUM: undef/missing mask for Signature Revocation List

struct DNSHeader {
  static const uint32_t QR_MASK = 0x8000;
  static const uint32_t OPCODE_MASK = 0x7800; // shr 11
  static const uint32_t AA_MASK = 0x0400;
  static const uint32_t TC_MASK = 0x0200;
  static const uint32_t RD_MASK = 0x0100;
  static const uint32_t RA_MASK = 0x0080;
  static const uint32_t RCODE_MASK = 0x000F;

  uint16_t msgID;
  uint16_t Bits;
  uint16_t QDCount;
  uint16_t ANCount;
  uint16_t NSCount;
  uint16_t ARCount;

  inline void Transcode() {
    for(uint16_t *p = (uint16_t*)(void*)&msgID; p <= (uint16_t*)(void*)&ARCount; p++)
      *p = ntohs(*p);
  }
} __attribute__((packed)); // struct DNSHeader

struct DNSAP {		// DNS Amplifier Protector ExpDecay structure
  uint16_t timestamp;	// Time in 64s ticks
  uint16_t temp;	// ExpDecay temperature
} __attribute__((packed));

struct Verifier {
    Verifier() : mask(VERMASK_NEW) {}	// -1 == uninited, neg != -1 == cant fetch
    int32_t  mask;		// Signature Revocation List mask
    string   srl_tpl;		// Signature Revocation List template
    CKeyID   keyID;		// Key for verify message
}; // 72 bytes = 18 words


struct TollFree {
    TollFree(const char *re) :
	regex(boost::xpressive::sregex::compile(string(re))), regex_str(re)
    {}
    boost::xpressive::sregex	regex;
    string			regex_str;
    vector<string>		e2u;
};

class MfcDns {
  public:
     MfcDns(const char *bind_ip, uint16_t port_no,
	    const char *gw_suffix, const char *allowed_suff, 
	    const char *local_fname, 
	    uint32_t dapsize, uint32_t daptreshold,
	    const char *enums, const char *tollfree, 
	    uint8_t verbose);
    ~MfcDns();

    void Run();

  private:
    static void StatRun(void *p);
    int  HandlePacket();
    uint16_t HandleQuery();
    int  Search(uint8_t *key);
    int  LocalSearch(const uint8_t *key, uint8_t pos, uint8_t step);
    int  Tokenize(const char *key, const char *sep2, char **tokens, char *buf);
    void Answer_ALL(uint16_t qtype, char *buf);
    void Answer_OPT();
    void Fill_RD_IP(char *ipddrtxt, int af);
    int  Fill_RD_DName(char *txt, uint8_t mxsz, int8_t txtcor); // return ref to name
    int  TryMakeref(uint16_t label_ref);

    // Handle Special function - phone number in the E.164 format
    // to support ENUM service
    int SpfunENUM(uint8_t len, uint8_t **domain_start, uint8_t **domain_end);
    // Generate answewr for found EMUM NVS record
    void Answer_ENUM(const char *q_str);
    void HandleE2U(char *e2u);
    bool CheckEnumSig(const char *q_str, char *sig_str);
    void AddTF(char *tf_tok);
    bool CheckDAP(void *key, int len, uint32_t packet_size);

    inline void Out2(uint16_t x) { x = htons(x); memcpy(m_snd, &x, 2); m_snd += 2; }
    inline void Out4(uint32_t x) { x = htonl(x); memcpy(m_snd, &x, 4); m_snd += 4; }
    void OutS(const char *p);

    DNSHeader *m_hdr; // 1st bzero element
    DNSAP    *m_dap_ht;	// Hashtable for DAP; index is hash(IP)
    char     *m_value;
    const char *m_gw_suffix;
    uint8_t  *m_buf, *m_bufend, *m_snd, *m_rcv, *m_rcvend;
    SOCKET    m_sockfd;
    int       m_rcvlen;
    uint32_t m_timestamp;
    uint32_t  m_daprand;	// DAP random value for universal hashing
    uint32_t  m_dapmask, m_dap_treshold;
    uint32_t  m_ttl;
    uint16_t  m_label_ref;
    uint16_t  m_gw_suf_len;
    char     *m_allowed_base;
    char     *m_local_base;
    int16_t   m_ht_offset[0x100]; // Hashtable for allowed TLD-suffixes(>0) and local names(<0)
    uint8_t   m_gw_suf_dots;
    uint8_t   m_allowed_qty;
    uint8_t   m_verbose;	// LAST bzero element
    int8_t    m_status;
    boost::thread m_thread;
    map<string, Verifier> m_verifiers;
    vector<TollFree>      m_tollfree;
}; // class MfcDns

#endif // MFCDNS_H
