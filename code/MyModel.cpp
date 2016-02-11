#include "MyModel.h"
#include "DNest4/code/Utils.h"
#include "Data.h"
#include <cmath>

using namespace std;
using namespace DNest4;

const Data& MyModel::data = Data::get_instance();

MyModel::MyModel()
{
:bursts(4, 100, false, ClassicMassInf1D(data.get_t_min(), data.get_t_max(),
                1E-10, 5.0*3.5e5*data.get_dt()))
,mu(data.get_t().size())
}

void MyModel::calculate_mu()
{
        const vector<double>& t_left = data.get_t_left();
        const vector<double>& t_right = data.get_t_right();

        // Update or from scratch?
        bool update = (bursts.get_added().size() < bursts.get_components().size());

        // Get the components
        const vector< vector<double> >& components = (update)?(bursts.get_added()):
                                (bursts.get_components());

        // Set the background level
        if(!update)
                mu.assign(mu.size(), background);

        double amplitude, skew, tc;
        double rise, fall;

        for(size_t j=0; j<components.size(); j++)
        {
                tc = components[j][0];
                amplitude = components[j][1];
                rise = components[j][2];
                skew = components[j][3];

                fall = rise*skew;

                for(size_t i=0; i<mu.size(); i++)
                {
                        if(tc <= t_left[i])
                        {
                                // Bin to the right of peak
                                mu[i] += amplitude*fall/data.get_dt()*
                                                (exp((tc - t_left[i])/fall) -
                                                 exp((tc - t_right[i])/fall));
                        }
                        else if(tc >= t_right[i])
                        {
                                // Bin to the left of peak
                                mu[i] += -amplitude*rise/data.get_dt()*
                                                (exp((t_left[i] - tc)/rise) -
                                                 exp((t_right[i] - tc)/rise));
                        }
                        else
                        {
                                // Part to the left
                                mu[i] += -amplitude*rise/data.get_dt()*
                                                (exp((t_left[i] - tc)/rise) -
                                                 1.);

                                // Part to the right
                                mu[i] += amplitude*fall/data.get_dt()*
                                                (1. -
                                                 exp((tc - t_right[i])/fall));
                        }
                }
        }

}

void MyModel::from_prior(RNG& rng)
{
    background = tan(M_PI*(0.97*randomU() - 0.485));
    background = exp(background);
    bursts.fromPrior();
        calculate_mu();

}

double MyModel::perturb(RNG& rng)
{
	double logH = 0.;

        if(randomU() <= 0.2)
        {
                for(size_t i=0; i<mu.size(); i++)
                        mu[i] -= background;

                background = log(background);
                background = (atan(background)/M_PI + 0.485)/0.97;
                background += pow(10., 1.5 - 6.*randomU())*randn();
                background = mod(background, 1.);
                background = tan(M_PI*(0.97*background - 0.485));
                background = exp(background);

                for(size_t i=0; i<mu.size(); i++)
                        mu[i] += background;
        }
        else
        {
                logH += bursts.perturb();
                bursts.consolidate_diff();
                calculate_mu();
        }



	return logH;
}

double MyModel::log_likelihood() const
{
	double logL = 0.;
        for(size_t i=0; i<y.size(); i++)
                logl += -mu[i] + y[i]*log(mu[i]) - gsl_sf_lngamma(y[i] + 1.);

	return logL;
}

void MyModel::print(std::ostream& out) const
{
        out<<background<<' ';
        bursts.print(out);
        for(size_t i=0; i<mu.size(); i++)
                out<<mu[i]<<' ';

}

string MyModel::description() const
{
	return string("");
}

