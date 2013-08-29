#include <librorc/registers.h>
#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_pg.hh>

#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>

#include <pda.h>

using namespace std;

/** extern error number **/
extern int errno;



namespace librorc
{

    dma_channel::dma_channel
    (
        uint32_t  channel_number,
        device   *dev,
        bar      *bar
    )
    : dma_channel(channel_number, dev, bar)
    {

    }

    dma_channel::dma_channel
    (
        uint32_t  channel_number,
        uint32_t  pcie_packet_size,
        device   *dev,
        bar      *bar,
        buffer   *eventBuffer,
        buffer   *reportBuffer
    )
    : dma_channel(channel_number, pcie_packet_size, dev, bar, eventBuffer, reportBuffer)
    {

    }

    dma_channel::~dma_channel()
    {
        if(m_reportBuffer != NULL)
        {
            m_reportBuffer->clear();
        }
    }

}
