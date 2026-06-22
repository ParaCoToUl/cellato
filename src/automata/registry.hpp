#ifndef CELLATO_AUTOMATA_REGISTRY_HPP
#define CELLATO_AUTOMATA_REGISTRY_HPP

#include "cellato/utils/type_list.hpp"

#include "brian/config.hpp"
#include "critters/config.hpp"
#include "cyclic/config.hpp"
#include "excitable/config.hpp"
#include "fire/config.hpp"
#include "fluid/config.hpp"
#include "game_of_life/config.hpp"
#include "maze/config.hpp"
#include "traffic/config.hpp"
#include "wire/config.hpp"

#define CELLATO_AUTOMATA(APPLY)                                                                                        \
    APPLY(game_of_life, game_of_life::config)                                                                          \
    APPLY(fire, fire::config)                                                                                          \
    APPLY(wire, wire::config)                                                                                          \
    APPLY(excitable, excitable::config)                                                                                \
    APPLY(brian, brian::config)                                                                                        \
    APPLY(maze, maze::config)                                                                                          \
    APPLY(fluid, fluid::config)                                                                                        \
    APPLY(critters, critters::config)                                                                                  \
    APPLY(cyclic, cyclic::config)                                                                                      \
    APPLY(traffic, traffic::config)

#define CELLATO_AUTOMATON_TYPE_LIST(short_name, config_type) cellato::utils::type_list<config_type>,

namespace cellato::automata {

using all = cellato::utils::concat_t<CELLATO_AUTOMATA(CELLATO_AUTOMATON_TYPE_LIST) cellato::utils::type_list<>>;

} // namespace cellato::automata

#undef CELLATO_AUTOMATON_TYPE_LIST

#endif // CELLATO_AUTOMATA_REGISTRY_HPP
