#pragma once

#include "cista/containers/hash_map.h"

#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp"

#include "utl/get_or_create.h"
#include "utl/verify.h"

#include "geo/latlng.h"

// source:
// https://github.com/motis-project/nigiri/commit/8c84336539fd02dfabd15e66f9f660ef577328e3

namespace geo {
struct proj_context {
  ~proj_context() { proj_context_destroy(ctx_); }

  operator PJ_CONTEXT*() const { return ctx_; }

  PJ_CONTEXT* ctx_{proj_context_create()};
};

struct proj_transformer {
  PJ_COORD transform(PJ_COORD const& c) const {
    return transformer_->transform(c);
  }

  osgeo::proj::crs::CRSNNPtr input_crs_;
  std::vector<osgeo::proj::operation::CoordinateOperationNNPtr> operations_;
  osgeo::proj::operation::CoordinateTransformerNNPtr transformer_;
};

// can only be used in a single thread
struct proj_transformers {
  proj_transformers()
      : db_ctx_{osgeo::proj::io::DatabaseContext::create()},
        auth_factory_{osgeo::proj::io::AuthorityFactory::create(db_ctx_, "")},
        auth_factory_epsg_{
            osgeo::proj::io::AuthorityFactory::create(db_ctx_, "EPSG")},
        coord_op_ctx_{
            osgeo::proj::operation::CoordinateOperationContext::create(
                auth_factory_, nullptr, 0.0)},
        coord_op_factory_{
            osgeo::proj::operation::CoordinateOperationFactory::create()},
        wgs84_crs_{
            auth_factory_epsg_->createCoordinateReferenceSystem("4326")} {}

  latlng transform(std::string const& input_crs_spec, double const input_y,
                   double const input_x) {
    if (input_crs_spec.empty()) {
      // assume WGS84 with lat, lon order
      return {input_y, input_x};
    }
    auto const& transformer =
        utl::get_or_create(transformers_, input_crs_spec, [&]() {
          try {
            auto crs_spec = input_crs_spec;
            auto input_crs =
                NN_CHECK_THROW(nn_dynamic_pointer_cast<osgeo::proj::crs::CRS>(
                    osgeo::proj::io::createFromUserInput(crs_spec, db_ctx_)));
            auto list = coord_op_factory_->createOperations(
                input_crs, wgs84_crs_, coord_op_ctx_);
            utl::verify(!list.empty(),
                        "No coordinate operations "
                        "found for {} -> WGS84",
                        crs_spec);
            auto coordinate_transformer =
                list[0]->normalizeForVisualization()->coordinateTransformer(
                    ctx_);
            return std::make_unique<proj_transformer>(proj_transformer{
                .input_crs_ = std::move(input_crs),
                .operations_ = std::move(list),
                .transformer_ = std::move(coordinate_transformer)});
          } catch (std::exception const& e) {
            std::cerr << "Error creating transformer for " << input_crs_spec
                      << ": " << e.what() << std::endl;
            return std::unique_ptr<proj_transformer>{};
          }
        });
    utl::verify(transformer != nullptr,
                "No transformer found for input CRS spec: {}", input_crs_spec);

    auto const output_coord =
        transformer->transform(PJ_COORD{{input_x, input_y, 0.0, HUGE_VAL}});
    return {output_coord.xy.y, output_coord.xy.x};
  }

  osgeo::proj::io::DatabaseContextNNPtr db_ctx_;
  osgeo::proj::io::AuthorityFactoryNNPtr auth_factory_;
  osgeo::proj::io::AuthorityFactoryNNPtr auth_factory_epsg_;
  osgeo::proj::operation::CoordinateOperationContextNNPtr coord_op_ctx_;
  osgeo::proj::operation::CoordinateOperationFactoryNNPtr coord_op_factory_;
  osgeo::proj::crs::CRSNNPtr wgs84_crs_;
  proj_context ctx_{};
  cista::raw::hash_map<std::string, std::unique_ptr<proj_transformer>>
      transformers_;
};

}  // namespace geo