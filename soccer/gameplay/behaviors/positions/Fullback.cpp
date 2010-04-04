#include "Fullback.hpp"

#include <Constants.hpp>
#include <vector>

#include <boost/foreach.hpp>

#include "../../Window.hpp"

using namespace std;

Gameplay::Behaviors::Fullback::Fullback(GameplayModule *gameplay, Side side):
	Behavior(gameplay, 1),
	_side(side)
{
}

bool Gameplay::Behaviors::Fullback::assign(set<Robot *> &available)
{
	_robots.clear();
	takeBest(available);
	
	//Initial state
	_state = Init;

	//initialize windowevaluator
	_winEval = new Gameplay::WindowEvaluator(Behavior::gameplay()->state());
	_winEval->debug = false;

	return _robots.size() >= _minRobots;
}

bool Gameplay::Behaviors::Fullback::run()
{
	if (!assigned() || !allVisible())
	{
		return false;
	}
	
	Geometry2d::Point ballFuture = ball().pos + ball().vel;

	//goal line, for intersection detection
	Geometry2d::Segment goalLine(Geometry2d::Point(-Constants::Field::GoalWidth / 2.0f, 0),
								  Geometry2d::Point(Constants::Field::GoalWidth / 2.0f, 0));

	// Update the target window
	_winEval->exclude.clear();
	_winEval->exclude.push_back(Behavior::robot()->pos());
	
	//exclude robots that arn't the fullback
	//_winEval->run(ball().pos, goalLine);
	
	BOOST_FOREACH(Fullback *f, otherFullbacks)
	{
		if (f->robot())
		{
			_winEval->exclude.push_back(f->robot()->pos());
		}
	}
	
	_winEval->run(ballFuture, goalLine);
	
	Window* best = 0;

	Behavior* goalie = _gameplay->goalie();
	
	bool needTask = false;
	
	//pick biggest window on appropriate side
	if (goalie && goalie->robot())
	{
		BOOST_FOREACH(Window* window, _winEval->windows)
		{
			if (_side == Left)
			{
				if (!best || window->segment.center().x < goalie->robot()->pos().x)
				{
					best = window;
				}
			}
			else if (_side == Right)
			{
				if (!best || window->segment.center().x > goalie->robot()->pos().x)
				{
					best = window;
				}
			}
		}
	}
	else
	{
		//if no side parameter...stay in the middle
		float bestDist = 0;
		BOOST_FOREACH(Window* window, _winEval->windows)
		{
			Geometry2d::Segment seg(window->segment.center(), ball().pos);
			float newDist = seg.distTo(Behavior::robot()->pos());
			
			if (!best || newDist < bestDist)
			{
				best = window;
				bestDist = newDist;
			}				
		}
	}
	
	if (best)
	{
		Geometry2d::Segment shootLine(ball().pos, ball().pos + ball().vel.normalized() * 7.0);
		
		Geometry2d::Segment& winSeg = best->segment;
		
		if (ball().vel.magsq() > 0.03 && winSeg.intersects(shootLine))
		{
			robot()->move(shootLine.nearestPoint(Behavior::robot()->pos()));
			robot()->faceNone();
		}
		else
		{
			const float winSize = winSeg.length();
				
			if (winSize < Constants::Ball::Radius)
			{
				needTask = true;
			}
			else
			{
				const float radius = .7;
				
				Geometry2d::Circle arc(Geometry2d::Point(), radius);
				
				Geometry2d::Line shot(winSeg.center(), ballFuture);
				Geometry2d::Point dest[2];
				
				bool intersected = shot.intersects(arc, &dest[0], &dest[1]);
				
				if (intersected)
				{
					if (dest[0].y > 0)
					{
						Behavior::robot()->move(dest[0]);
					}
					else
					{
						Behavior::robot()->move(dest[1]);
					}
					Behavior::robot()->face(ballFuture);
				}
				else
				{
					needTask = true;
				}
			}
		}
	}
	else
	{
		needTask = true;
	}
	
	return false;
}

float Gameplay::Behaviors::Fullback::score(Robot* robot)
{
	//robot closest to the back line wins
	return robot->pos().y;
}
