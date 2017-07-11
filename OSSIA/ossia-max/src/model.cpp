#include "model.hpp"
#include "parameter.hpp"
//#include "view.hpp"
//#include "remote.hpp"
#include <ossia/network/base/node_attributes.hpp>

using namespace ossia::max;
    
# pragma mark -
# pragma mark ossia_model class methods

extern "C"
void ossia_model_setup()
{
    auto& ossia_library = ossia_max::instance();
    
    // instantiate the ossia.parameter class
    ossia_library.ossia_model_class = class_new("ossia.model",
                                                (method)ossia_model_new,
                                                (method)ossia_model_free,
                                                (long)sizeof(ossia::max::t_model),
                                                0L, A_GIMME, 0);
    
    class_addmethod(ossia_library.ossia_model_class, (method)ossia_model_assist,     "assist",       A_CANT, 0);
    
    CLASS_ATTR_SYM(ossia_library.ossia_model_class, "description", 0, t_model, m_description);
    CLASS_ATTR_SYM(ossia_library.ossia_model_class, "tags",        0, t_model, m_tags);
    
    class_register(CLASS_BOX, ossia_library.ossia_model_class);
}

extern "C"
void* ossia_model_new(t_symbol *name, long argc, t_atom *argv)
{
    auto& ossia_library = ossia_max::instance();
    t_model* x = (t_model *) object_alloc(ossia_library.ossia_model_class);
    
    if (x)
    {
        // make outlets
        x->m_dump_out = outlet_new(x, NULL);						// anything outlet to dump client state
        
        x->m_description = _sym_nothing;
        x->m_tags = _sym_nothing;
        
        x->m_regclock = clock_new(x, (method)object_register<t_model>);
        
        // parse arguments
        long attrstart = attr_args_offset(argc, argv);
        
        // check name argument
        x->m_name = _sym_nothing;
        if (attrstart && argv)
        {
            if (atom_gettype(argv) == A_SYM)
            {
                x->m_name = atom_getsym(argv);
                x->m_absolute = std::string(x->m_name->s_name) != "" && x->m_name->s_name[0] == '/';
            }
        }
        
        if (x->m_name == _sym_nothing)
        {
            object_error((t_object*)x, "needs a name as first argument");
            x->m_name = gensym("untitledModel");
            return x;
        }
        
        // process attr args, if any
        attr_args_process(x, argc-attrstart, argv+attrstart);
        
        // we need to delay registration because object may use patcher hierarchy to check address validity
        // and object will be added to patcher's objects list (aka canvas g_list) after model_new() returns.
        // 0 ms delay means that it will be perform on next clock tick
        clock_delay(x->m_regclock, 0);
    }
    
    return (x);
}

extern "C"
void ossia_model_free(t_model *x)
{
    x->m_dead = true;
    x->unregister();
    object_dequarantining<t_model>(x);
    object_free(x->m_regclock);
}

extern "C"
void ossia_model_assist(t_model *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET)
    {
        sprintf(s, "I am inlet %ld", a);
    }
    else
    {
        sprintf(s, "I am outlet %ld", a);
    }
}

namespace ossia {
namespace max {
        
# pragma mark -
# pragma mark t_model structure functions
    
    bool t_model :: register_node(ossia::net::node_base* node)
    {
        bool res = do_registration(node);
        
        if (res)
        {
            object_dequarantining<t_model>(this);
            
            std::vector<box_hierachy> children = find_children_to_register(&m_object, get_patcher(&m_object), gensym("ossia.model"));
            
            for (auto child : children)
            {
                if (child.classname == gensym("ossia.model"))
                {
                    t_model* model = (t_model*)jbox_get_object(child.box);
                    
                    // ignore itself
                    if (model == this)
                        continue;
                    
                    model->register_node(m_node);
                }
                else if (child.classname == gensym("ossia.parameter"))
                {
                    t_parameter* parameter = (t_parameter*)jbox_get_object(child.box);
                    
                    parameter->register_node(m_node);
                }
            }
/*
            for (auto view : t_view::quarantine())
            {
                object_register<t_view>(static_cast<t_view*>(view));
            }
            
            // then try to register qurantinized remote
            for (auto remote : t_remote::quarantine())
            {
                object_register<t_remote>(static_cast<t_remote*>(remote));
            }
 */
        }
        else
            object_quarantining<t_model>(this);
        
        return res;
    }
    
    bool t_model :: do_registration(ossia::net::node_base* node)
    {
        // already register to this node
        if (m_node && m_node->get_parent() == node)
            return true;
        
        // we should unregister here because we may have add a node between the registered node and the parameter
        unregister();
       
        if (!node)
            return false;
        
        // check if a node with the same name already exists to avoid auto-incrementing name
        if (node->find_child(m_name->s_name))
        {
            std::vector<box_hierachy> children_model = find_children_to_register(&m_object, get_patcher(&m_object), gensym("ossia.model"));
            
            for (auto child : children_model)
            {
                if (child.classname == gensym("ossia.parameter"))
                {
                    t_parameter* parameter = (t_parameter*)jbox_get_object(child.box);
                    
                    // if we already have a t_parameter node of that name, unregister it
                    // we will register it again after node creation
                    if (parameter->m_name == m_name)
                    {
                        parameter->unregister();
                        continue;
                    }
                }
            }
        }
        
        m_node = &ossia::net::create_node(*node, m_name->s_name);
        m_node->about_to_be_deleted.connect<t_model, &t_model::is_deleted>(this);
        
        ossia::net::set_description(*m_node, m_description->s_name);
        ossia::net::set_tags(*m_node, parse_tags_symbol(m_tags));
        
        return true;
    }
    
    bool t_model :: unregister()
    {
        // not registered
        if (!m_node)
            return true;
        
        if (m_node && m_node->get_parent())
            m_node->get_parent()->remove_child(*m_node); // this calls isDeleted() on each registered child and put them into quarantine
        
        m_node = nullptr;
        
        object_quarantining<t_model>(this);
        
        register_quarantinized();
        
        return true;
    }
    
    void t_model :: is_deleted(const ossia::net::node_base& n)
    {
        if (!m_dead)
        {
            m_node->about_to_be_deleted.disconnect<t_model, &t_model::is_deleted>(this);
            m_node = nullptr;
            object_quarantining<t_model>(this);
        }
    }
    
    bool t_model :: is_renamed(t_model* x)
    {
        return ossia::contains(x->rename(), x);
    }
    
    void t_model :: renaming(t_model* x)
    {
        if (!is_renamed(x)) x->rename().push_back(x);
    }
    
    void t_model :: derenaming(t_model* x)
    {
        x->rename().erase(std::remove(x->rename().begin(), x->rename().end(), x), x->rename().end());
    }
    
} // max namespace
} // ossia namespace