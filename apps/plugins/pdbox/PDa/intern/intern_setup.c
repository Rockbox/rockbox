#include "../src/m_pd.h"

#ifndef ROCKBOX
#include <stdio.h>
#endif

#ifdef ROCKBOX
/* Get rid of warnings. */
void biquad_tilde_setup(void);
void bp_tilde_setup(void);
void clip_tilde_setup(void);
void cos_tilde_setup(void);
void dbtopow_tilde_setup(void);
void dbtorms_tilde_setup(void);
void delread_tilde_setup(void);
void delwrite_tilde_setup(void);
void env_tilde_setup(void);
void ftom_tilde_setup(void);
void hip_tilde_setup(void);
void line_tilde_setup(void);
void lop_tilde_setup(void);
void mtof_tilde_setup(void);
void noise_tilde_setup(void);
void osc_tilde_setup(void);
void phasor_tilde_setup(void);
void powtodb_tilde_setup(void);
void print_tilde_setup(void);
void rmstodb_tilde_setup(void);
void rsqrt_tilde_setup(void);
void samphold_tilde_setup(void);
void sfread_tilde_setup(void);
void sfwrite_tilde_setup(void);
void sig_tilde_setup(void);
void snapshot_tilde_setup(void);
void sqrt_tilde_setup(void);
void tabosc4_tilde_setup(void);
void tabplay_tilde_setup(void);
void tabread4_tilde_setup(void);
void tabread_tilde_setup(void);
void tabread_setup(void);
void tabreceive_tilde_setup(void);
void tabsend_tilde_setup(void);
void tabwrite_tilde_setup(void);
void tabwrite_setup(void);
void threshold_tilde_setup(void);
void vcf_tilde_setup(void);
void vd_tilde_setup(void);
void vline_tilde_setup(void);
void vsnapshot_tilde_setup(void);
void wrap_tilde_setup(void); 
#endif /* ROCKBOX */

void d_intern_setup(void) {
#ifndef ROCKBOX
    fprintf(stderr,"setup\n");
#endif
    biquad_tilde_setup();
    bp_tilde_setup();
    clip_tilde_setup();
    cos_tilde_setup();
    dbtopow_tilde_setup();
    dbtorms_tilde_setup();
    delread_tilde_setup();
    delwrite_tilde_setup();
    env_tilde_setup();
    ftom_tilde_setup();
    hip_tilde_setup();
    line_tilde_setup();
    lop_tilde_setup();
    mtof_tilde_setup();
    noise_tilde_setup();
    osc_tilde_setup();
    phasor_tilde_setup();
    powtodb_tilde_setup();
    print_tilde_setup();
    rmstodb_tilde_setup();
    rsqrt_tilde_setup();
    samphold_tilde_setup();
    sfread_tilde_setup();
    sfwrite_tilde_setup();
    sig_tilde_setup();
    snapshot_tilde_setup();
    sqrt_tilde_setup();
    tabosc4_tilde_setup();
    tabplay_tilde_setup();
    tabread4_tilde_setup();
    tabread_tilde_setup();
    tabread_setup();
    tabreceive_tilde_setup();
    tabsend_tilde_setup();
    tabwrite_tilde_setup();
    tabwrite_setup();
    threshold_tilde_setup();
    vcf_tilde_setup();
    vd_tilde_setup();
    vline_tilde_setup();
    vsnapshot_tilde_setup();
    wrap_tilde_setup(); 
}

