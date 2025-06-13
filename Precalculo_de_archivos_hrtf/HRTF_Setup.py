from pysofaconventions import * # For getting measurement locations and FIRs from SOFA files
from scipy.spatial import Delaunay # For triangulating the measurement points, provides data for pathing algorithm
import numpy as np # math
import json
import struct

#---------------------------------------------------------------------------------------------------------------------------
#   INICIO DEL PROGRAMA
#---------------------------------------------------------------------------------------------------------------------------

if True:
    def setupHRTF():
        folderPath = ''
        fileNames = [
            'HRIR_L2354',
            'HRIR_L2354'
        ]
        #Se duplica los archivos para almacenar una copia de cada dato para hacer una cascara uniforme (Se pueden duplicar los datos luego de obtener la lista)
        sofaFiles = [SOFAFile(folderPath+fileName+'.sofa','r') for fileName in fileNames]
        # Crea una lista de objetos SOFA

        sourcePositions = np.concatenate([sofaFile.getVariableValue('SourcePosition')
            for sofaFile in sofaFiles])
        # Crea por obtencion y concatenacion una matriz de [N, 3] con N posiciones de todos los archivos SOFA
        # Columna 0: Az | Columna 1: El | Columna 2: R
    
        sourcePositions[:,:2] *= np.pi/180
        # Convierte todas los datos de Az y El a radianes

        cullAmount = 3

        meanFreePath = 4*max(sourcePositions[:,2])/np.sqrt(len(sourcePositions)/cullAmount)
        # max(sourcePositions[:,2]) -> maximo valor en la columna 2 de sourcePositions[:], es decir, el maximo radio de todas las posiciones
        # len(sourcePositions)/cullAmount -> Se pretende triangular una cantidad de puntos mas reducida
        # 4*max(...)/np.sqrt(...) -> Estima la distancia promedio entre los puntos en una esfera
        
        sourcePositions[len(sourcePositions)//2:,2] += meanFreePath
        # La mitad de los puntos (copias de originales) se desplazan para formar una esfera mas grande como cascara

        maxR = max(sourcePositions[:,2])-meanFreePath/2
        # Obtener el radio maximo seguro
        
        FIRs = np.concatenate([sofaFile.getDataIR()
            for sofaFile in sofaFiles])
        FIRs = np.asarray(FIRs)
        # FIRs es una array [N, 2, M] donde N es el numero de mediciones (puntos), 2 correspondiendo a
        # los canales (L & R) y M el numero de muestras (longitud de cada filtro)

        az = np.array(sourcePositions[:,0])
        el = np.array(sourcePositions[:,1])
        r = np.array(sourcePositions[:,2])
        # az, el y r son listas que contienen la respectiva columna de todos los datos

        xs = np.sin(az)*np.cos(el)*r
        ys = np.cos(az)*np.cos(el)*r
        zs = np.sin(el)*r
        # Se calcula el punto (xs, ys, zs) de cada posicion de las listas

        points = np.array([xs, ys, zs]).transpose()
        # se unen los datos en un array [3, N] y se transpone para tener estructura [N, 3]

        sourcePositions = sourcePositions[::cullAmount]
        FIRs = FIRs[::cullAmount]
        points = points[::cullAmount]
        # lista[inicio:fin:paso]
        # Guarda cada valor cada ciertos pasos, en este caso cada cullAmount pasos ya que se pretende calcular una cantidad de puntos reducida

        tri = Delaunay(points, qhull_options="QJ Pp")
        # Variaxiones para evitar puntos perfectamente alineados

        tetraCoords = points[tri.simplices]
        # tri.simplices es un array [N, 4] donde N es el numero de tetraedros y 4 son los indices correspondientes a los vertices de cada tetraedro
        # tetraCoords guarda, para cada tetraedro en tri.simplices, las coordenadas de points correspondientes a sus vertices
        # tetraCoords es una matriz [N, 4, 3]
        # ----------------------------------------------------------------------
        # EJEMPLO: tetraedro 0 = [[2, 6, 7, 9]]
        #   tetraCoords = [
        #   Tetraedro 0 -> [points[2],
        #                   points[6],
        #                   points[7],
        #                   points[9]],
        #   Tetraedro 1 -> [...],
        #                   ...]

        T = np.transpose(np.array((tetraCoords[:,0]-tetraCoords[:,3],
                    tetraCoords[:,1]-tetraCoords[:,3],
                    tetraCoords[:,2]-tetraCoords[:,3])), (1,0,2))
        # Para cada tetraedro obtiene el vector de las aristas formadas por su cuarto vertice con el resto
        # Se almacenan en un array [3, N, 3], (3 aristas, N tetraedros, 3 coordenadas (x, y, z) de cada arista)
        #   Fila 0: V0 - V3
        #   Fila 1: V1 - V3
        #   Fila 2: V2 - V3
        # Luego realiza la transpuesta con (1, 0, 2) para reordenar y obtener una estructura [N, 3, 3] (nº Tetraedros, aristas, coordenadas)

        def fast_inverse(A):
            # Calcula las inversas de las matrices 3x3 de cada tetraedro
            identity = np.identity(A.shape[2], dtype=A.dtype)
            # Matriz identidad de tamaño A.shape[2] = 3
            Ainv = np.zeros_like(A)
            # Copia A con los valores a 0
            planarCount=0
            # Recuento de matrices no invertibles (tetraedros degenerados)
            for i in range(A.shape[0]):
                # Para cada tetraedro
                try:
                    Ainv[i] = np.linalg.solve(A[i], identity)
                    # Resuelve AX = I (Igual a calcular la inversa de A)
                except np.linalg.LinAlgError:
                    # Si la matriz es singular (det = 0) incrementar el contador
                    planarCount += 1
            # print(planarCount)
            return Ainv

        Tinv = fast_inverse(T)
        # Array [M, 3, 3] con las matrices inversas de cada tetraedro (tetraedros degenerados a 0)
        return(tetraCoords, Tinv, tri, FIRs, maxR)


    def save_to_sd_format(tetraCoords, Tinv, tri, FIRs, maxR):
        # 1. Crear directorio si no existe
        import os
        if not os.path.exists('teensy_data'):
            os.makedirs('teensy_data')
        
        # 2. Guardar metadatos en JSON
        config = {
            'num_tetrahedrons': len(tetraCoords),
            'num_positions': FIRs.shape[0],
            'fir_length': FIRs.shape[2],
            'max_radius': float(maxR),
            'min_radius': 0.075,
            'data_types': {
                'tetraCoords': 'float32',
                'Tinv': 'float32',
                'simplices': 'uint16',
                'FIRs': 'int16',
                'neighbors': 'int16'
            }
        }
        
        with open('teensy_data3/config.json', 'w') as f:
            json.dump(config, f, indent=2)
        
        # 3. Función auxiliar para guardar arrays binarios
        def save_binary(array, filename, dtype):
            with open(f'teensy_data3/{filename}', 'wb') as f:
                if dtype == 'float32':
                    array.astype(np.float32).tofile(f)
                elif dtype == 'uint16':
                    array.astype(np.uint16).tofile(f)
                elif dtype == 'int16':
                    array.astype(np.int16).tofile(f)
        
        # 4. Guardar todos los arrays
        save_binary(tetraCoords, 'tetraCoords.bin', 'float32')
        save_binary(Tinv, 'Tinv.bin', 'float32')
        save_binary(tri.simplices, 'tri_simplices.bin', 'uint16')
        # Antes de guardar FIRs.bin
        FIRs_quantized = np.int16(FIRs * 32767)  # Conversión a fixed-point 16-bit
        save_binary(FIRs_quantized, 'FIRs.bin', 'int16')  # Guardas como int16
        save_binary(tri.neighbors, 'neighbors.bin', 'int16')
        
        print("Datos guardados en formato Teensy en la carpeta 'teensy_data3'")


    tetraCoords, Tinv, tri, FIRs, maxR = setupHRTF()
    save_to_sd_format(tetraCoords, Tinv, tri, FIRs, maxR)

