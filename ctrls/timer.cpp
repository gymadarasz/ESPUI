const char* jsCtrlTimerInit = R"TIMER_SCRIPT(
    (app) => {
        if (app.timers) {
            throw "Timers already initialized.";
        }
        app.timers = {
            instances: {},
            stop: (name) => {
                if (!app.timers.instances[name]) {
                    throw "Timer instance with name: '" + name + "' is not exists.";
                }
                clearInterval(app.timers.instances[name].interval);
                delete app.timers.instances[name].interval;
            }
        };
    }
)TIMER_SCRIPT";

const char* jsCtrlTimer = R"TIMER_SCRIPT(
    (app) => {
        if (app.timers.instances.{{ name }}) {
            throw "A timer instance with name '{{ name }}' already exists.";
        }
        app.timers.instances.{{ name }} = {
            interval: setInterval(() => {
                {{ onTick }}
            }, {{ period }})
        };
    }
)TIMER_SCRIPT";