#ifndef FSM_CFG_HPP
#define FSM_CFG_HPP

#include "cfg/basic_cfg.hpp"
#include "cfg/cfg_algorithms.hpp"
#include "cfg/cfg_load.hpp"
#include "cfg/cfg_transform.hpp"

#include <string>

namespace fsm
{
using cfg = basic_cfg<std::string>;
} // namespace fsm

#endif // FSM_CFG_HPP
