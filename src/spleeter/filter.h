#ifndef SPLEETER_FILTER_H_
#define SPLEETER_FILTER_H_

#include <system_error>
#include <mutex>

#include "artff/abstract_filter.h"

#include "spleeter/tf_handle.h"
#include "spleeter/type.h"

namespace spleeter {

using Bundle = std::pair<TFHandlePtr<TF_Session>, TFHandlePtr<TF_Graph>>;
using BundlePtr = std::shared_ptr<Bundle>;

class Filter : public artff::AbstractFilter {
public:
  Filter(SeparationType separation_type);

  /// Initialize the filter with Spleeter specific options
  void Init(std::error_code& err);

  // ------
  // FILTER PARAMETERS
  /// Set the volume of a given stem
  /// \param stem_index TODO: give the stem name according to process type
  /// \param value the volume value (0 <= volume <= 1)
  void set_volume(uint8_t stem_index, float value);
  float volume(uint8_t stem_index) const;

  // ----------------------------------------------------------------------
  // ALGORITHM PARAMETERS

  /// Set the Neural Network input size
  /// \param size
  /// \note reducing that value will reduce the latency but also reduces the
  /// temporal information and will lower the quality
  void set_ProcessLength(uint16_t size);
  uint16_t ProcessLength() const;

  /// Set the number of frames processed at a time
  /// \param size
  /// \note Always <= ProcessLength
  void set_FrameLength(uint16_t size);
  uint16_t FrameLength() const;

  /// Set the cross fade frame count between each frames. It helps reducing the
  /// inconsistency between independent processes
  /// \param size
  void set_OverlapLength(uint16_t size);
  uint16_t OverlapLength() const;

  /// Using Spleeter, The sum of each stem may not be conservative. We can force
  /// it by deviding each mask by the mask sum
  /// \param value
  void set_ForceConservativity(bool value);
  bool ForceConservativity() const;

  /// Every time we run a process, we will do it on ProcessLength.
  /// However, if we decide to reduce FrameLength, it will get more CPU
  /// intensive as we will process more often but it will reduce the latency.
  /// Latency is T - (T - FrameLength) / 2
  /// If FrameLength = 1, latency is ~T/2 as we always need to process the
  /// center of the matrix. It is the one that beneficit the most of temporal
  /// information.
  uint32_t FrameLatency() const override;


private:
  void PrepareToPlay() override;
  void AsyncProcessTransformedBlock(std::vector<std::complex<float>*> data,
                                    uint32_t size) override;

  uint32_t SpleeterFrameLatency() const;

private:
  SeparationType m_type;
  BundlePtr m_bundle;
  std::vector<float> m_volumes;

  uint16_t m_process_length;
  uint16_t m_frame_length;
  uint16_t m_overlap_length;
  bool m_force_conservativity;

  // ------------
  // internal buffers
  // -- The frame being processed
  uint32_t m_frame_index;
  // -- in
  TFHandlePtr<TF_Tensor> m_network_input;
  TFHandlePtr<TF_Tensor> m_previous_network_input;
  std::vector<int64_t> m_shapes;
  // -- out
  std::vector<TFHandlePtr<TF_Tensor>> m_network_result;
  std::vector<TFHandlePtr<TF_Tensor>> m_previous_network_result;
  // -- for single frame processing
  std::vector<std::vector<float>> m_mask_vec_data;
  std::vector<std::vector<float>> m_previous_mask_vec_data;
  std::vector<std::vector<float>> m_mask_sum_vec_data;
  std::vector<float*> m_mask_data;
  std::vector<float*> m_previous_mask_data;
  std::vector<float*> m_mask_sum_data;
  std::vector<std::vector<std::vector<float>>> m_masks_vec_data;
  std::vector<std::vector<float*>> m_masks_data;
  
  std::mutex m_mutex;
};

} // namespace spleeter

#endif // SPLEETER_FILTER_H_
