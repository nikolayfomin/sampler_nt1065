# The application version inclusion script

# Пытаемся вытянуть номер версии из последнего тэга
GIT_VERSION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)

# Неработающий (по крайней мере для msvc2010) код проверки наличия номера версии
#VERSION = 0.2.0
##!contains(GIT_VERSION,\d+\.\d+\.\d+) {
#    # If there is nothing we simply use version defined manually
#    isEmpty(GIT_VERSION) {
#        GIT_VERSION = $$VERSION
#    } else { # otherwise construct proper git describe string
#        GIT_COMMIT_COUNT = $$system(git --git-dir $$PWD/.git --work-tree $$PWD  rev-list HEAD --count)
#        isEmpty(GIT_COMMIT_COUNT) {
#            GIT_COMMIT_COUNT = 0
#        }
#        GIT_VERSION = $$VERSION-$$GIT_COMMIT_COUNT
#    }
#}

## Превращаем "tag-commits-hash" в "tag.commits.hash"
GIT_VERSION ~= s/-/"."
GIT_VERSION ~= s/g/""
## Удаляем хэш
GIT_VERSION ~= s/.[a-f0-9]{6,}//

VERSION = $$GIT_VERSION

# Задаём макрос для использования константы в приложении
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
