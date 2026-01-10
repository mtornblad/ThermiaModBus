Import("env")

def reset_target(source, target, env):
    # Ändra /dev/ttyAMA1 till din port om den är annorlunda
    # -r triggar reset
    cmd = "pymcuprog reset -d avr64dd28 -t uart -u /dev/ttyAMA1"
    print(f"Executing: {cmd}")
    env.Execute(cmd)

# Registrera kommandot "reset"
env.AddCustomTarget(
    name="reset",
    dependencies=None,
    actions=[reset_target],
    title="Pymcuprog Reset",
    description="Resets the AVR64DD28 using pymcuprog"
)