#pragma once

#include "kodo/rlnc/full_vector_codes.hpp"
#include "kodo/rlnc/on_the_fly_codes.hpp"
#include "kodo/sparse_uniform_generator.hpp"
#include "kodo/storage_aware_generator.hpp"
#include "kodo/partial_decoding_tracker.hpp"
#include "kodo/rank_info.hpp"
#include "kodo/payload_rank_encoder.hpp"
#include "kodo/payload_rank_decoder.hpp"
#include "kodo/largest_nonzero_index_decoder.hpp"


namespace kodo {

typedef fifi::binary field;

template<class Field>
class track_on_the_fly_encoder :
    public // Payload Codec API
           //payload_rank_encoder<
           payload_encoder<
           // Codec Header API
           systematic_encoder<
           symbol_id_encoder<
           // Symbol ID API
           plain_symbol_id_writer<
           // Coefficient Generator API
           storage_aware_generator<
           uniform_generator<
           // Codec API
           encode_symbol_tracker<
           zero_symbol_encoder<
           linear_block_encoder<
           storage_aware_encoder<
           //rank_info<
           // Coefficient Storage API
           coefficient_info<
           // Symbol Storage API
           deep_symbol_storage<
           storage_bytes_used<
           storage_block_info<
           // Finite Field API
           finite_field_math<typename fifi::default_field<Field>::type,
           finite_field_info<Field,
           // Factory API
           final_coder_factory_pool<
           // Final type
           track_on_the_fly_encoder<Field>
           > > > > > > > > > > > > > > > > > //> >
{ };

template<class MainStack>
class track_on_the_fly_recoding_stack
    : public // Payload API
             //payload_rank_encoder<
             payload_encoder<
             // Codec Header API
             non_systematic_encoder<
             symbol_id_encoder<
             // Symbol ID API
             recoding_symbol_id<
             // Coefficient Generator API
             uniform_generator<
             // Codec API
             encode_symbol_tracker<
             zero_symbol_encoder<
             linear_block_encoder<
             //rank_info<
             // Proxy
             proxy_layer<
             track_on_the_fly_recoding_stack<MainStack>,
             MainStack> > > > > > > > > //> >
{ };

template<class Field>
class track_on_the_fly_decoder :
    public // Payload API
           partial_decoding_tracker<
           payload_recoder<track_on_the_fly_recoding_stack,
           //payload_rank_decoder<
           payload_decoder<
           // Codec Header API
           systematic_decoder<
           symbol_id_decoder<
           largest_nonzero_index_decoder<
           // Symbol ID API
           plain_symbol_id_reader<
           // Codec API
           aligned_coefficients_decoder<
           forward_linear_block_decoder<
           rank_info<
           // Coefficient Storage API
           coefficient_storage<
           coefficient_info<
           // Storage API
           deep_symbol_storage<
           storage_bytes_used<
           storage_block_info<
           // Finite Field API
           finite_field_math<typename fifi::default_field<Field>::type,
           finite_field_info<Field,
           // Factory API
           final_coder_factory_pool<
           // Final type
           track_on_the_fly_decoder<Field>
           > > > > > > > > > > > > > > > > > >
{ };

typedef on_the_fly_encoder<field> encoder;
typedef on_the_fly_decoder<field> decoder;
typedef on_the_fly_decoder<field> recoder;

}  // namespace kodo
