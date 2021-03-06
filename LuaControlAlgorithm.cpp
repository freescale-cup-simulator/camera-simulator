#include <LuaControlAlgorithm.h>
#include <QDebug>

LuaControlAlgorithm::LuaControlAlgorithm(QObject *parent)
    : IControlAlgorithm(parent)
    , m_lua_loaded(false)
    , m_interval(100)
{

}

LuaControlAlgorithm::~LuaControlAlgorithm()
{
    lua_close(m_lua_state);
}

bool LuaControlAlgorithm::loadFile(const QString &file)
{
    Q_ASSERT(!m_lua_loaded);

    m_lua_loaded = (luaL_loadfile(m_lua_state, file.toLocal8Bit().data()) == 0);
    if (!m_lua_loaded)
    {
        qWarning() << "Load error: " << lua_tostring(m_lua_state, -1);
        lua_pop(m_lua_state, 1);
        return false;
    }

    lua_pushvalue(m_lua_state, -1);
    lua_pcall(m_lua_state, 0, 0, 0);

    lua_pushnumber(m_lua_state, m_interval);
    lua_setglobal(m_lua_state, "g_run_interval");

    qDebug() << "Script successfully loaded";
    return true;
}

QString LuaControlAlgorithm::getId()
{
    return "Lua Control algoritgm (file: " + m_current_file + ")";
}

void LuaControlAlgorithm::setInterval(int msec)
{
    m_interval = msec;
}

bool LuaControlAlgorithm::executeControlScript(const QByteArray & frame)
{
    Q_ASSERT(m_lua_loaded);

    // important: make a copy of the loaded function, pcall will pop it
    lua_pushvalue(m_lua_state, -1);

    lua_newtable(m_lua_state);
    for (int i = 0; i < frame.size(); i++)
    {
        lua_pushnumber(m_lua_state, i + 1);
        lua_pushnumber(m_lua_state, static_cast<unsigned char>(frame.at(i)));
        lua_settable(m_lua_state, -3);
    }
    lua_setglobal(m_lua_state, "g_camera_frame");

    if (lua_pcall(m_lua_state, 0, LUA_MULTRET, 0) != 0)
    {
        qDebug() << "Lua error: " << lua_tostring(m_lua_state, -1);
        lua_pop(m_lua_state, 1);
        return false;
    }

    DataSet control_data;

    lua_getfield(m_lua_state, -1, "angle");
    control_data["angle"] = lua_tonumber(m_lua_state, -1);
    lua_pop(m_lua_state, 1);

    lua_getfield(m_lua_state, -1, "lspeed");
    control_data["lspeed"] = lua_tonumber(m_lua_state, -1);
    lua_pop(m_lua_state, 1);

    lua_getfield(m_lua_state, -1, "rspeed");
    control_data["rspeed"] = lua_tonumber(m_lua_state, -1);
    lua_pop(m_lua_state, 1);

    lua_pop(m_lua_state, 1);
    controlSignal(control_data);

    return true;
}

void LuaControlAlgorithm::onCameraResponse(const QByteArray &data)
{
    //for(int i=0;i<data.length();i++)
    //    qDebug()<<(unsigned char)data.at(i);
    Q_ASSERT(executeControlScript(data));
}

void LuaControlAlgorithm::init()
{
    m_lua_state=luaL_newstate();
    luaL_openlibs(m_lua_state);
    loadFile("./lua/simple_algorithm.lua");
}
