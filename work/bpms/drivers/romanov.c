
enum
{
    NADC_CHAN_COUNT = 100,
    DELY_CHAN_COUNT = 10,
    GAIN_CHAN_COUNT = 10,
    DCMN_CHAN_COUNT = 20,
    TXZI_CHAN_COUNT = 100,
    CONF_CHAN_COUNT = 50,

    DELY_CHAN_POSIT = 12,
};

/*
 * calculate the position of "delay" channel in the whole map of channels
 * here bpm_num_id is the numerical id of BPM in the booster which should
 * strictly lay in range of [1..16]
 */

int get_dely_chan(const int bpm_num_id)
{
  int val;

    val = NADC_CHAN_COUNT * 16 +
          DELY_CHAN_COUNT * 4  +
          GAIN_CHAN_COUNT * 2  +
          DCMN_CHAN_COUNT * 2  +
          TXZI_CHAN_COUNT * 16 +
          CONF_CHAN_COUNT * (bpm_num_id - 1) +
          GAIN_CHAN_POSIT;

    retunrn val;
}

void set_dely(const int bpm_num_id, int delay_in_picos)
{
  int chan = get_dely_chan(bpm_num_id);

  ~/pult/bin/cx-setchan ring1:35 chan=delay_in_picos
}
