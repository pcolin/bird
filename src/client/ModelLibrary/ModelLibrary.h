#pragma once

#include "..\..\share\model\BlackScholesMertonModel.h"
#include "..\..\share\model\CoxRossRubinsteinModel.h"
#include "..\..\share\model\VolatilityModel.h"

using namespace System;

namespace ModelLibrary {

public ref struct GreeksDataWrapper
{
	double spot;
	double vol;
	double theo;
	double delta;
	double gamma;
	double theta;
	double vega;
	double rho1;
	double rho2;
	double vol_theta;
	double rate_theta;

	GreeksDataWrapper(const Model::GreeksData &data, double v)
		: spot(data.spot), vol(v), theo(data.theo), delta(data.delta), gamma(data.gamma), theta(data.theta), vega(data.vega),
		rho1(data.rho1), rho2(data.rho2), vol_theta(data.vol_theta), rate_theta(data.rate_theta)
	{}
};

public ref struct TheoDataWrapper
{
	double theo;
	double delta;
	double gamma;
	double theta;

	TheoDataWrapper(const Model::TheoData &data)
		: theo(data.theo), delta(data.delta), gamma(data.gamma), theta(data.theta)
	{}
};

public ref class PricingModelWrapper
{
public:

	PricingModelWrapper(bool european)
	{
		if (european)
		{
			model_ = new Model::BlackScholesMertonModel();
		}
		else
		{
			model_ = new Model::CoxRossRubinsteinModel();
		}
	}

	~PricingModelWrapper()
	{
		this->!PricingModelWrapper();
	}

	!PricingModelWrapper()
	{
		if (model_)
		{
			delete model_;
			model_ = nullptr;
		}
	}

	double Calculate(bool call, double s, double k, double v, double r, double q, double t)
	{
		return model_->Calculate(call, s, k, v, r, q, t);
	}

	GreeksDataWrapper^ CalculateGreeks(bool call, double s, double k, double v, double r, double q, double t)
	{
		Model::GreeksData data;
		model_->Calculate(call, s, k, v, r, q, t, data);
		return gcnew GreeksDataWrapper(data, v);
	}

	TheoDataWrapper^ CalculateTheo(bool call, double s, double k, double v, double r, double q, double t)
	{
		Model::TheoData data;
		model_->Calculate(call, s, k, v, r, q, t, data);
		return gcnew TheoDataWrapper(data);
	}

	double CalculateIV(bool call, double v, double p, double s, double k, double r, double q, double t)
	{
		return model_->CalculateIV(call, v, p, s, k, r, q, t);
	}

private:
	Model::PricingModel *model_ = nullptr;
};

public ref struct VolatilityParameterWrapper
{
	double spot;
	double skew;
	double atm_vol;
	double call_convex;
	double put_convex;
	double call_slope;
	double put_slope;
	double call_cutoff;
	double put_cutoff;
	double vcr;
	double scr;
	double ccr;
	double spcr;
	double sccr;
};

public ref class VolatilityModelWrapper
{
public:
	VolatilityModelWrapper() : model_(new Model::VolatilityModel())
	{}

	~VolatilityModelWrapper()
	{
		this->!VolatilityModelWrapper();
	}

	!VolatilityModelWrapper()
	{
		if (model_)
		{
			delete model_;
			model_ = nullptr;
		}
	}

	double Calculate(const VolatilityParameterWrapper ^param, bool future, double s, double k, double r, double q, double t)
	{
		Model::VolatilityModel::Parameter p;
		p.spot = param->spot;
		p.skew = param->skew;
		p.atm_vol = param->atm_vol;
		p.call_convex = param->call_convex;
		p.put_convex = param->put_convex;
		p.call_slope = param->call_slope;
		p.put_slope = param->put_slope;
		p.call_cutoff = param->call_cutoff;
		p.put_cutoff = param->put_cutoff;
		p.vcr = param->vcr;
		p.scr = param->scr;
		p.ccr = param->ccr;
		p.spcr = param->spcr;
		p.sccr = param->sccr;
		return model_->Calculate(p, future, s, k, r, q, t);
	}

private:
	Model::VolatilityModel *model_;
};

}
