#ifndef CLIENT_HH
#define CLIENT_HH
#include <fstream>
#include "socket.hh"
#include "srslte/srslte.h"
#include "win_minmax.h"

#define cell_prb_len NOF_REPORT_SF
#define ue_tbs_len 10

class Client
{
private:
    FILE* _log_file;
    const Socket _send;
    Socket::Address _remote;
   
    uint32_t _pkt_received;
    uint32_t _max_ack_number;
    int	     _nof_cell;
    uint32_t _last_tti;
    int	     _last_rate;

    int	     _last_exp_rate;
    int	     _last_est_rate;
    uint64_t _start_time;
    uint32_t _switch2wire_time_us;
    uint32_t _switched_t_threshold_us;
    float    _switch_drain_factor;
    bool     _switch2wire;

    struct   minmax win_delay_us; 
    uint32_t _delay_window_us;
    int	     _nof_delayed_pkt;
    int	     _nof_normal_pkt;

    int	     _blk_ack; 
    bool     _256QAM;
    int	     _BER_sel;

    bool     _slow_start;
    float    _overhead_factor;    	
public:
    Client( const Socket & s_send, 
	    const Socket::Address & s_remote,
	    FILE* _file_fd);	
    
    int	    cellMaxPrb[MAX_NOF_CA];
    int	    last_tbs[MAX_NOF_CA];
    int	    last_tbs_hm[MAX_NOF_CA];
    bool    ca_active[MAX_NOF_CA];
    int	    cell_dl_prb[MAX_NOF_CA][NOF_REPORT_SF];
    int	    ue_dl_prb[MAX_NOF_CA][NOF_REPORT_SF];
    int	    mcs_tb1[MAX_NOF_CA][NOF_REPORT_SF];
    int	    mcs_tb2[MAX_NOF_CA][NOF_REPORT_SF];
    int	    tbs[MAX_NOF_CA][NOF_REPORT_SF];
    int	    tbs_hm[MAX_NOF_CA][NOF_REPORT_SF];

    void    init_connection( void );
    void    close_connection( void );
    void    recv( void );
    void    recv_noRF( srslte_lteCCA_rate* );
    void    recv_noRF_fixRate();
    void    recv_noRF_noACK( );

    void    set_cell_num_prb( void );
    void    set_blk_ack(int);
    void    set_256QAM(bool);
    void    set_ber_sel(int );
    void    set_overhead_factor(float);
    int	    get_rcvPkt( void );
        
    void    set_remote( const Socket::Address & s_remote ) { _remote = s_remote; }

//    Client( const Client & ) = delete;
//    const Client & operator=( const Client & ) = delete;
};

#endif
